#include "txeo/Logger.h"
#include "txeo/detail/PredictorPrivate.h"
#include "txeo/detail/TensorHelper.h"
#include "txeo/detail/TensorPrivate.h"
#include "txeo/detail/utils.h"

#include <tensorflow/cc/saved_model/tag_constants.h>
#include <tensorflow/core/framework/tensor.h>
#include <utility>

namespace tf = tensorflow;

namespace txeo {

template <typename T>
void Predictor<T>::load_model() {
  std::unordered_set<std::string> tags{static_cast<const char *>(tf::kSavedModelTagServe)};
  tensorflow::ConfigProto *config = &_impl->session_options.config;
  config->mutable_gpu_options()->set_allow_growth(true);

  tf::Status status = tf::LoadSavedModel(_impl->session_options, _impl->run_options,
                                         _impl->model_path, tags, &_impl->model);
  if (!status.ok())
    throw PredictorError("Error loading model: " + status.ToString());

  auto meta_graph_def = _impl->model.meta_graph_def;
  auto signature_map = meta_graph_def.signature_def().at("serving_default");

  if (signature_map.inputs().size() == 0)
    throw PredictorError("The loaded model has no input metadata!");
  if (signature_map.outputs().size() == 0)
    throw PredictorError("The loaded model has no output metadata!");

  for (const auto &input : signature_map.inputs()) {
    auto info = input.second;
    if (info.has_name() && info.has_tensor_shape())
      _impl->in_name_shape_map.emplace_back(
          info.name(), detail::proto_to_txeo_tensor_shape(*info.mutable_tensor_shape()));
    else if (info.has_name())
      _impl->in_name_shape_map.emplace_back(info.name(), TensorShape({0}));
  }

  for (const auto &output : signature_map.outputs()) {
    auto info = output.second;
    if (info.has_name() && info.has_tensor_shape())
      _impl->out_name_shape_map.emplace_back(
          info.name(), detail::proto_to_txeo_tensor_shape(*info.mutable_tensor_shape()));
    else if (info.has_name())
      _impl->out_name_shape_map.emplace_back(info.name(), TensorShape({0}));
  }

  _logger->info("Model loaded successfully");
}

template <typename T>
Predictor<T>::Predictor(std::filesystem::path model_path, txeo::Logger &logger)
    : _impl{std::make_unique<Impl>()}, _logger{&logger} {
  _impl->model_path = model_path;
  this->load_model();
}

template <typename T>
Predictor<T>::~Predictor() {
  auto aux = _impl->model.session->Close();
}

template <typename T>
const Predictor<T>::TensorInfo &Predictor<T>::get_input_metadata() const noexcept {
  return _impl->in_name_shape_map;
}

template <typename T>
const Predictor<T>::TensorInfo &Predictor<T>::get_output_metadata() const noexcept {
  return _impl->out_name_shape_map;
}

template <typename T>
Tensor<T> Predictor<T>::predict(const Tensor<T> &input) const {
  if (_impl->in_name_shape_map[0].second.axis_dim(0) != 0) {
    if (_impl->in_name_shape_map[0].second != input.shape())
      throw PredictorError("The shape of the input tensor and the model input do not match!");
  } else {
    if (_impl->in_name_shape_map[0].second.number_of_axes() != input.order())
      throw PredictorError("The shape of the input tensor and the model input do not match!");
    for (int i{1}; i < input.order(); ++i) {
      if (_impl->in_name_shape_map[0].second.axis_dim(i) != input.shape().axis_dim(i))
        throw PredictorError("The shape of the input tensor and the model input do not match!");
    }
  }

  const auto &input_name = _impl->in_name_shape_map[0].first;
  const auto &output_name = _impl->out_name_shape_map[0].first;
  const auto &tf_tensor = *input._impl->tf_tensor;
  std::vector<tf::Tensor> outputs;

  _logger->info("Prediction started...");

  auto status = _impl->model.session->Run({{input_name, tf_tensor}}, {output_name}, {}, &outputs);
  if (!status.ok())
    PredictorError("Error running model: " + status.ToString());

  _logger->info("Prediction finished...");

  auto resp = detail::TensorHelper::to_txeo_tensor<T>(std::move(outputs[0]));

  return resp;
}

template <typename T>
std::optional<TensorShape> Predictor<T>::get_input_metadata_shape(const std::string &name) const {
  for (auto &item : _impl->in_name_shape_map)
    if (item.first == name)
      return item.second;
  return std::nullopt;
}

template <typename T>
std::optional<TensorShape> Predictor<T>::get_output_metadata_shape(const std::string &name) const {
  for (auto &item : _impl->out_name_shape_map)
    if (item.first == name)
      return item.second;
  return std::nullopt;
}

template <typename T>
std::vector<Tensor<T>> Predictor<T>::predict_batch(const Predictor<T>::TensorIdent &inputs) const {
  for (size_t i{0}; i < inputs.size(); ++i) {
    auto shp = this->get_input_metadata_shape(inputs[i].first);
    if (!shp)
      throw PredictorError("An input name could not be found!");
    if (shp->axis_dim(0) != 0)
      if (shp != inputs[i].second.shape())
        throw PredictorError("The shape of an input tensor and the model input do not match!");
  }

  std::vector<std::pair<std::string, tf::Tensor>> tf_inputs;
  for (size_t i{0}; i < inputs.size(); ++i)
    tf_inputs.emplace_back(inputs[i].first, *inputs[i].second._impl->tf_tensor);

  std::vector<tf::Tensor> outputs;
  auto output_name = _impl->out_name_shape_map[0].first;

  _logger->info("Batch prediction started...");

  auto status = _impl->model.session->Run(tf_inputs, {output_name}, {}, &outputs);
  if (!status.ok())
    PredictorError("Error running model: " + status.ToString());

  _logger->info("Batch prediction finished...");

  std::vector<Tensor<T>> resp;
  for (auto &item : outputs)
    resp.emplace_back(detail::TensorHelper::to_txeo_tensor<T>(std::move(item)));

  return resp;
}

template <typename T>
void Predictor<T>::enable_xla(bool enable) {
  _impl->session_options.config.mutable_graph_options()
      ->mutable_optimizer_options()
      ->set_global_jit_level(enable ? tensorflow::OptimizerOptions::ON_1
                                    : tensorflow::OptimizerOptions::OFF);
  auto aux = _impl->model.session->Close();
  this->load_model();
}

template <typename T>
std::vector<DeviceInfo> Predictor<T>::get_devices() const {
  std::vector<tensorflow::DeviceAttributes> devices;
  tensorflow::Status status = _impl->model.session->ListDevices(&devices);
  std::vector<DeviceInfo> resp;
  for (auto &item : devices) {
    DeviceInfo aux{.name = item.name(),
                   .device_type = item.device_type(),
                   .memory_limit = detail::to_size_t(item.memory_limit())};
    resp.emplace_back(aux);
  }

  return resp;
};

template class Predictor<short>;
template class Predictor<int>;
template class Predictor<long>;
template class Predictor<long long>;
template class Predictor<float>;
template class Predictor<double>;

} // namespace txeo