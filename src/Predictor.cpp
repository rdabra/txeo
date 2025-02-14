#include "txeo/Predictor.h"
#include "txeo/Tensor.h"
#include "txeo/TensorShape.h"
#include "txeo/detail/TensorPrivate.h"

#include "txeo/detail/PredictorPrivate.h"
#include "txeo/detail/utils.h"
#include <cstddef>
#include <memory>
#include <tensorflow/cc/saved_model/tag_constants.h>
#include <tensorflow/core/framework/tensor_shape.h>

namespace tf = tensorflow;

namespace txeo {

template <typename T>
Predictor<T>::Predictor(std::string model_path) : _impl{std::make_unique<Impl>()} {
  std::unordered_set<std::string> tags{static_cast<const char *>(tf::kSavedModelTagServe)};

  tf::Status status = tf::LoadSavedModel(_impl->session_options, _impl->run_options, model_path,
                                         tags, &_impl->model);
  if (!status.ok())
    throw txeo::PredictorError("Error loading model: " + status.ToString());

  auto meta_graph_def = _impl->model.meta_graph_def;
  auto signature_map = meta_graph_def.signature_def().at("serving_default");

  for (const auto &input : signature_map.inputs()) {
    auto info = input.second;
    if (info.has_name() && info.has_tensor_shape())
      _impl->in_name_shape_map.emplace_back(
          info.name(), txeo::detail::proto_to_txeo_tensor_shape(*info.mutable_tensor_shape()));
    else if (info.has_name())
      _impl->in_name_shape_map.emplace_back(info.name(), txeo::TensorShape({0}));
  }

  for (const auto &output : signature_map.outputs()) {
    auto info = output.second;
    if (info.has_name() && info.has_tensor_shape())
      _impl->out_name_shape_map.emplace_back(
          info.name(), txeo::detail::proto_to_txeo_tensor_shape(*info.mutable_tensor_shape()));
    else if (info.has_name())
      _impl->out_name_shape_map.emplace_back(info.name(), txeo::TensorShape({0}));
  }
}

template <typename T>
inline Predictor<T>::~Predictor() {
  auto aux = _impl->model.session->Close();
}

template <typename T>
inline const Predictor<T>::TensorInfo &Predictor<T>::get_input_metadata() const {
  return _impl->in_name_shape_map;
}

template <typename T>
inline const Predictor<T>::TensorInfo &Predictor<T>::get_output_metadata() const {
  return _impl->out_name_shape_map;
}

template <typename T>
inline txeo::Tensor<T> Predictor<T>::predict(const txeo::Tensor<T> input) const {
  if (_impl->in_name_shape_map.size() == 0)
    throw txeo::PredictorError("The loaded model has no input metadata!");
  if (_impl->out_name_shape_map.size() == 0)
    throw txeo::PredictorError("The loaded model has no output metadata!");
  if (_impl->in_name_shape_map[0].second.axis_dim(0) != 0)
    if (_impl->in_name_shape_map[0].second != input.shape())
      throw txeo::PredictorError("The shape of the input tensor and the model input do not match!");

  auto input_name = _impl->in_name_shape_map[0].first;
  auto output_name = _impl->out_name_shape_map[0].first;
  auto tf_tensor = *input._impl->tf_tensor;
  std::vector<tf::Tensor> outputs;

  auto status = _impl->model.session->Run({{input_name, tf_tensor}}, {output_name}, {}, &outputs);
  if (!status.ok())
    txeo::PredictorError("Error running model: " + status.ToString());

  txeo::Tensor<T> resp{txeo::detail::to_txeo_tensor<T>(outputs[0])};

  return resp;
}

template <typename T>
inline std::optional<txeo::TensorShape>
Predictor<T>::get_input_metadata_shape(const std::string &name) const {
  for (auto &item : _impl->in_name_shape_map)
    if (item.first == name)
      return item.second;
  return std::nullopt;
}

template <typename T>
inline std::optional<txeo::TensorShape>
Predictor<T>::get_output_metadata_shape(const std::string &name) const {
  for (auto &item : _impl->out_name_shape_map)
    if (item.first == name)
      return item.second;
  return std::nullopt;
}

template <typename T>
std::vector<txeo::Tensor<T>> Predictor<T>::predict(const Predictor<T>::TensorIdent &inputs) const {
  if (_impl->out_name_shape_map.size() == 0)
    throw txeo::PredictorError("The loaded model has no output metadata!");
  for (size_t i{0}; i < inputs.size(); ++i) {
    auto shp = this->get_input_metadata_shape(inputs[i].first);
    if (!shp)
      throw txeo::PredictorError("An input name could not be found!");
    if (shp->axis_dim(0) != 0)
      if (shp != inputs[i].second.shape())
        throw txeo::PredictorError(
            "The shape of an input tensor and the model input do not match!");
  }

  std::vector<std::pair<std::string, tf::Tensor>> tf_inputs;
  for (size_t i{0}; i < inputs.size(); ++i)
    tf_inputs.emplace_back(inputs[i].first, *inputs[i].second._impl->tf_tensor);

  std::vector<tf::Tensor> outputs;
  auto output_name = _impl->out_name_shape_map[0].first;

  auto status = _impl->model.session->Run(tf_inputs, {output_name}, {}, &outputs);
  if (!status.ok())
    txeo::PredictorError("Error running model: " + status.ToString());

  std::vector<txeo::Tensor<T>> resp;
  for (auto &item : outputs)
    resp.emplace_back(txeo::detail::to_txeo_tensor<T>(item));

  return resp;
}

template class Predictor<short>;
template class Predictor<int>;
template class Predictor<long>;
template class Predictor<long long>;
template class Predictor<float>;
template class Predictor<double>;

} // namespace txeo