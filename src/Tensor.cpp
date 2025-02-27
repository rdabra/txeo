
#include "txeo/TensorOp.h"
#include "txeo/TensorShape.h"
#include "txeo/detail/TensorPrivate.h"
#include "txeo/detail/TensorShapePrivate.h"
#include "txeo/detail/utils.h"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <tensorflow/core/framework/tensor.h>
#include <tensorflow/core/framework/tensor_shape.h>
#include <utility>
#include <vector>

namespace txeo {

namespace tf = tensorflow;

template <typename T>
void Tensor<T>::check_indexes(const std::vector<size_t> &indexes) {
  for (size_t i{0}; i < indexes.size(); ++i) {
    auto aux = txeo::detail::to_int64(indexes[i]);
    if (aux < 0 || aux >= _impl->txeo_shape.axis_dim(i))
      throw TensorError("Index out of bounds!");
  }
}

template <typename T>
template <typename P>
void Tensor<T>::create_from_shape(P &&shape) {
  auto aux = std::forward<P>(shape);
  auto shp = aux._impl->tf_shape != nullptr ? *aux._impl->tf_shape : *aux._impl->ext_tf_shape;
  _impl->tf_tensor = std::make_unique<tf::Tensor>(txeo::detail::get_tf_dtype<T>(), shp);
  _impl->txeo_shape._impl->ext_tf_shape = &_impl->tf_tensor->shape();
  _impl->txeo_shape._impl->stride =
      txeo::detail::calc_stride(*_impl->txeo_shape._impl->ext_tf_shape);
}

template <typename T>
Tensor<T>::Tensor() : _impl{std::make_unique<Impl>()} {
}

template <typename T>
Tensor<T>::Tensor(const Tensor &tensor) : _impl{std::make_unique<Impl>()} {
  create_from_shape(tensor.shape().clone());
  for (size_t i{0}; i < this->dim(); ++i)
    this->data()[i] = tensor.data()[i];
}

template <typename T>
Tensor<T>::Tensor(Tensor &&tensor) noexcept : _impl{std::make_unique<Impl>()} {
  if (this != &tensor) {
    _impl->tf_tensor = std::move(tensor._impl->tf_tensor);
    _impl->txeo_shape = std::move(tensor._impl->txeo_shape);
  }
}

// Defined here after "Impl" implementation in order to avoid incompleteness
template <typename T>
Tensor<T>::~Tensor() = default;

template <typename T>
Tensor<T>::Tensor(const txeo::TensorShape &shape) : _impl{std::make_unique<Impl>()} {
  this->create_from_shape(shape);
}

template <typename T>
Tensor<T>::Tensor(txeo::TensorShape &&shape) : _impl{std::make_unique<Impl>()} {
  this->create_from_shape(std::move(shape));
}

template <typename T>
Tensor<T>::Tensor(const std::vector<size_t> &shape) : _impl{std::make_unique<Impl>()} {
  this->create_from_shape(txeo::TensorShape(shape));
}

template <typename T>
Tensor<T>::Tensor(std::vector<size_t> &&shape) : _impl{std::make_unique<Impl>()} {
  this->create_from_shape(txeo::TensorShape(std::move(shape)));
}

template <typename T>
Tensor<T>::Tensor(const txeo::TensorShape &shape, const T &fill_value)
    : _impl{std::make_unique<Impl>()} {
  this->create_from_shape(shape);
  this->fill(fill_value);
}

template <typename T>
Tensor<T>::Tensor(txeo::TensorShape &&shape, const T &fill_value)
    : _impl{std::make_unique<Impl>()} {
  this->create_from_shape(std::move(shape));
  this->fill(fill_value);
}

template <typename T>
Tensor<T>::Tensor(const std::vector<size_t> &shape, const T &fill_value)
    : _impl{std::make_unique<Impl>()} {
  this->create_from_shape(txeo::TensorShape(shape));
  this->fill(fill_value);
}

template <typename T>
Tensor<T>::Tensor(std::vector<size_t> &&shape, const T &fill_value)
    : _impl{std::make_unique<Impl>()} {
  this->create_from_shape(txeo::TensorShape(std::move(shape)));
  this->fill(fill_value);
}

template <typename T>
Tensor<T>::Tensor(const txeo::TensorShape &shape, const std::vector<T> &values)
    : _impl{std::make_unique<Impl>()} {
  if (values.size() != shape.calculate_capacity())
    throw txeo::TensorError("Shape and number of values are incompatible!");
  create_from_shape(shape);
  std::copy(std::begin(values), std::end(values), this->data());
}

template <typename T>
Tensor<T>::Tensor(const std::vector<size_t> &shape, const std::vector<T> &values)
    : _impl{std::make_unique<Impl>()} {
  txeo::TensorShape aux(shape);
  if (values.size() != aux.calculate_capacity())
    throw txeo::TensorError("Shape and number of values are incompatible!");
  create_from_shape(aux);
  std::copy(std::begin(values), std::end(values), this->data());
}

template <typename T>
Tensor<T>::Tensor(const std::initializer_list<std::initializer_list<T>> &values)
    : _impl{std::make_unique<Impl>()} {
  std::vector<T> flat_data;
  std::vector<size_t> shape;
  this->fill_data_shape(values, flat_data, shape);
  create_from_shape(txeo::TensorShape(shape));
  std::copy(std::begin(flat_data), std::end(flat_data), this->data());
}

template <typename T>
Tensor<T>::Tensor(
    const std::initializer_list<std::initializer_list<std::initializer_list<T>>> &values)
    : _impl{std::make_unique<Impl>()} {
  std::vector<T> flat_data;
  std::vector<size_t> shape;
  this->fill_data_shape(values, flat_data, shape);
  create_from_shape(txeo::TensorShape(shape));
  std::copy(std::begin(flat_data), std::end(flat_data), this->data());
}

template <typename T>
Tensor<T> &Tensor<T>::operator=(const Tensor &tensor) {
  this->create_from_shape(tensor.shape().clone());
  for (size_t i{0}; i < this->dim(); ++i)
    this->data()[i] = tensor.data()[i];

  return *this;
}

template <typename T>
Tensor<T> &Tensor<T>::operator=(Tensor &&tensor) noexcept {
  if (this != &tensor) {
    _impl->tf_tensor = std::move(tensor._impl->tf_tensor);
    _impl->txeo_shape = std::move(tensor._impl->txeo_shape);
  }
  return (*this);
}

template <typename T>
bool Tensor<T>::operator==(const Tensor &tensor) {
  if (_impl->tf_tensor->shape() != tensor._impl->tf_tensor->shape())
    return false;
  for (size_t i{0}; i < this->dim(); ++i) {
    if (this->data()[i] != tensor.data()[i])
      return false;
  }

  return true;
}

template <typename T>
bool Tensor<T>::operator!=(const Tensor &tensor) {
  if (_impl->tf_tensor->shape() != tensor._impl->tf_tensor->shape())
    return true;
  for (size_t i{0}; i < this->dim(); ++i) {
    if (this->data()[i] != tensor.data()[i])
      return true;
  }

  return false;
}

template <typename T>
const txeo::TensorShape &Tensor<T>::shape() const {
  return _impl->txeo_shape;
}

template <typename T>
int Tensor<T>::order() const {
  return _impl->txeo_shape.number_of_axes();
}

template <typename T>
size_t Tensor<T>::dim() const {
  return _impl->txeo_shape.calculate_capacity();
}

template <typename T>
size_t Tensor<T>::memory_size() const {
  return _impl->tf_tensor->TotalBytes();
}

template <typename T>
const T *Tensor<T>::data() const {
  return static_cast<T *>(_impl->tf_tensor->data());
}

template <typename T>
T &Tensor<T>::operator()() {
  return this->data()[0];
}

template <typename T>
T &Tensor<T>::at() {
  if (this->order() != 0)
    throw TensorError("This tensor is not a scalar.");
  return (*this)();
}

template <typename T>
const T &Tensor<T>::operator()() const {
  return this->data()[0];
}

template <typename T>
const T &Tensor<T>::at() const {
  if (this->order() != 0)
    throw TensorError("This tensor is not a scalar.");
  return (*this)();
}

template <typename T>
void Tensor<T>::reshape(const txeo::TensorShape &shape) {
  auto old_tensor = std::move(_impl->tf_tensor);
  create_from_shape(shape);
  if (!_impl->tf_tensor->CopyFrom(*old_tensor, _impl->tf_tensor->shape()))
    throw txeo::TensorError("The number of axes do not match the dimension of this tensor!");
}

template <typename T>
void Tensor<T>::reshape(const std::vector<size_t> &shape) {
  reshape(txeo::TensorShape(shape));
}

template <typename T>
Tensor<T> Tensor<T>::slice(size_t first_axis_begin, size_t first_axis_end) const {
  if (first_axis_end < first_axis_begin)
    throw txeo::TensorError("The end index can not be less than the initial index!");
  if (txeo::detail::to_int64(first_axis_end) >= _impl->txeo_shape.axis_dim(0))
    throw txeo::TensorError(
        "The end index can not be greater than or equal to the dimension of first axis!");

  auto t_slice = _impl->tf_tensor->Slice(first_axis_begin, first_axis_end);
  Tensor<T> resp{txeo::detail::to_txeo_tensor_shape(t_slice.shape())};
  if (!resp._impl->tf_tensor->CopyFrom(t_slice, t_slice.shape()))
    throw txeo::TensorError("This tensor could not be sliced!");

  return resp;
}

template <typename T>
void Tensor<T>::view_of(const Tensor<T> &tensor, const txeo::TensorShape &shape) {
  if (this->dim() == 0)
    return;
  if (this->dim() != tensor.dim() || this->dim() != shape.calculate_capacity())
    throw txeo::TensorError("Parameters do not match the dimension of this tensor!");
  this->reshape(shape);
  if (!_impl->tf_tensor->CopyFrom(*tensor._impl->tf_tensor, *shape._impl->tf_shape))
    throw txeo::TensorError("This tensor could not be shared!");
}

template <typename T>
Tensor<T> Tensor<T>::flatten() const {
  Tensor<T> resp(txeo::TensorShape({this->dim()}));
  if (this->dim() != 0)
    if (!resp._impl->tf_tensor->CopyFrom(*_impl->tf_tensor, resp._impl->tf_tensor->shape()))
      throw txeo::TensorError("This tensor could not be flatten!");

  return resp;
}

template <typename T>
void Tensor<T>::fill(const T &value) {
  for (size_t i{0}; i < this->dim(); ++i)
    this->data()[i] = value;
}

template <typename T>
Tensor<T> &Tensor<T>::operator=(const T &value) {
  this->fill(value);

  return (*this);
}

template <typename T>
T *Tensor<T>::data() {
  return static_cast<T *>(_impl->tf_tensor->data());
}

template <typename T>
void Tensor<T>::shuffle() {
  if (this->dim() == 0)
    return;
  std::mt19937_64 engine{std::random_device{}()};

  auto *data = this->data();
  std::shuffle(data, data + this->dim(), engine);
}

template <typename T>
void Tensor<T>::squeeze() {
  std::vector<size_t> new_shape;
  const auto &aux = this->shape().axes_dims();
  for (auto &item : aux)
    if (item != 1)
      new_shape.emplace_back(txeo::detail::to_size_t(item));

  this->reshape(txeo::TensorShape(new_shape));
}

template <typename T>
Tensor<T> Tensor<T>::clone() const {
  Tensor<T> resp{*this};
  return resp;
}

template <typename U>
std::ostream &operator<<(std::ostream &os, const Tensor<U> &tensor) {
  os << *tensor._impl->tf_tensor;
  return os;
}

template <typename T>
void Tensor<T>::fill_with_uniform_random(const T &min, const T &max, const size_t &seed1,
                                         const size_t &seed2) {
  if (this->dim() == 0)
    return;
  if (max <= min)
    throw txeo::TensorError("The max value is not greater than the min value");

  auto aux_min = static_cast<double>(min);
  auto aux_max = static_cast<double>(max);

  std::mt19937 engine{};
  std::seed_seq sseq{seed1, seed2};
  engine.seed(sseq);

  std::uniform_real_distribution<double> scaler{aux_min, aux_max};
  for (size_t i{0}; i < this->dim(); ++i)
    this->data()[i] = static_cast<T>(scaler(engine));
}

template <typename T>
void Tensor<T>::fill_with_uniform_random(const T &min, const T &max) {
  if (this->dim() == 0)
    return;
  if (max <= min)
    throw txeo::TensorError("The max value is not greater than the min value");

  auto aux_min = static_cast<double>(min);
  auto aux_max = static_cast<double>(max);

  std::mt19937 engine{std::random_device{}()};

  std::uniform_real_distribution<double> scaler{aux_min, aux_max};
  for (size_t i{0}; i < this->dim(); ++i)
    this->data()[i] = static_cast<T>(scaler(engine));
}

template <typename U>
txeo::Tensor<U> operator+(const txeo::Tensor<U> &left, const txeo::Tensor<U> &right) {
  return TensorOp<U>::sum(left, right);
};

template <typename U>
txeo::Tensor<U> operator+(const txeo::Tensor<U> &left, const U &right) {
  return TensorOp<U>::sum(left, right);
};

template <typename U>
txeo::Tensor<U> operator-(const txeo::Tensor<U> &left, const txeo::Tensor<U> &right) {
  return TensorOp<U>::subtract(left, right);
};

template <typename U>
txeo::Tensor<U> operator-(const U &left, const txeo::Tensor<U> &right) {
  return TensorOp<U>::subtract(left, right);
};

template <typename U>
txeo::Tensor<U> operator-(const txeo::Tensor<U> &left, const U &right) {
  return TensorOp<U>::subtract(left, right);
};

template <typename U>
txeo::Tensor<U> operator*(const txeo::Tensor<U> &tensor, const U &scalar) {
  return TensorOp<U>::multiply(tensor, scalar);
};

template <typename U>
txeo::Tensor<U> operator/(const txeo::Tensor<U> &tensor, const U &scalar) {
  return TensorOp<U>::divide(tensor, scalar);
};

template <typename U>
txeo::Tensor<U> operator/(const U &left, const txeo::Tensor<U> &right) {
  return TensorOp<U>::divide(left, right);
};

template <typename T>
Tensor<T> &Tensor<T>::operator+=(const Tensor<T> &tensor) {
  txeo::TensorOp<T>::sum_by(*this, tensor);
  return *this;
}

template <typename T>
Tensor<T> &Tensor<T>::operator-=(const Tensor<T> &tensor) {
  txeo::TensorOp<T>::subtract_by(*this, tensor);
  return *this;
}

template <typename T>
Tensor<T> &Tensor<T>::operator*=(const T &scalar) {
  txeo::TensorOp<T>::multiply_by(*this, scalar);
  return *this;
}

template <typename T>
inline Tensor<T> &Tensor<T>::operator/=(const T &scalar) {
  txeo::TensorOp<T>::divide_by(*this, scalar);
  return *this;
}

template <typename T>
inline Tensor<T> &Tensor<T>::operator+=(const T &scalar) {
  txeo::TensorOp<T>::sum_by(*this, scalar);
  return *this;
}

template <typename T>
inline Tensor<T> &Tensor<T>::operator-=(const T &scalar) {
  txeo::TensorOp<T>::subtract_by(*this, scalar);
  return *this;
}

template <typename T>
Tensor<T> &Tensor<T>::hadamard_prod_by(const Tensor<T> &tensor) {
  txeo::TensorOp<T>::hadamard_prod_by(*this, tensor);

  return *this;
}

template <typename T>
Tensor<T> &Tensor<T>::hadamard_div_by(const Tensor<T> &tensor) {
  txeo::TensorOp<T>::hadamard_div_by(*this, tensor);

  return *this;
}

template <typename T>
Tensor<T> &Tensor<T>::power_elem_by(const T &exponent) {
  txeo::TensorOp<T>::power_elem_by(*this, exponent);

  return *this;
}

template <typename T>
txeo::TensorIterator<T> Tensor<T>::begin() {
  return txeo::TensorIterator<T>{this->data()};
}

template <typename T>
txeo::TensorIterator<T> Tensor<T>::end() {
  return txeo::TensorIterator<T>{this->data() + this->dim()};
}

template <typename T>
txeo::TensorIterator<const T> Tensor<T>::begin() const {
  return txeo::TensorIterator<const T>{this->data()};
}

template <typename T>
txeo::TensorIterator<const T> Tensor<T>::end() const {
  return txeo::TensorIterator<const T>{this->data() + this->dim()};
}

// Avoiding problems in linking

template class Tensor<short>;
template class Tensor<int>;
template class Tensor<bool>;
template class Tensor<long>;
template class Tensor<long long>;
template class Tensor<float>;
template class Tensor<double>;

template std::ostream &operator<<(std::ostream &, const Tensor<short> &);
template std::ostream &operator<<(std::ostream &, const Tensor<int> &);
template std::ostream &operator<<(std::ostream &, const Tensor<bool> &);
template std::ostream &operator<<(std::ostream &, const Tensor<long> &);
template std::ostream &operator<<(std::ostream &, const Tensor<long long> &);
template std::ostream &operator<<(std::ostream &, const Tensor<float> &);
template std::ostream &operator<<(std::ostream &, const Tensor<double> &);

template txeo::Tensor<short> operator+(const txeo::Tensor<short> &, const txeo::Tensor<short> &);
template txeo::Tensor<int> operator+(const txeo::Tensor<int> &, const txeo::Tensor<int> &);
template txeo::Tensor<bool> operator+(const txeo::Tensor<bool> &, const txeo::Tensor<bool> &);
template txeo::Tensor<long> operator+(const txeo::Tensor<long> &, const txeo::Tensor<long> &);
template txeo::Tensor<long long> operator+(const txeo::Tensor<long long> &,
                                           const txeo::Tensor<long long> &);
template txeo::Tensor<float> operator+(const txeo::Tensor<float> &, const txeo::Tensor<float> &);
template txeo::Tensor<double> operator+(const txeo::Tensor<double> &, const txeo::Tensor<double> &);

template txeo::Tensor<short> operator+(const txeo::Tensor<short> &, const short &);
template txeo::Tensor<int> operator+(const txeo::Tensor<int> &, const int &);
template txeo::Tensor<bool> operator+(const txeo::Tensor<bool> &, const bool &);
template txeo::Tensor<long> operator+(const txeo::Tensor<long> &, const long &);
template txeo::Tensor<long long> operator+(const txeo::Tensor<long long> &, const long long &);
template txeo::Tensor<float> operator+(const txeo::Tensor<float> &, const float &);
template txeo::Tensor<double> operator+(const txeo::Tensor<double> &, const double &);

template txeo::Tensor<short> operator-(const txeo::Tensor<short> &, const txeo::Tensor<short> &);
template txeo::Tensor<int> operator-(const txeo::Tensor<int> &, const txeo::Tensor<int> &);
template txeo::Tensor<bool> operator-(const txeo::Tensor<bool> &, const txeo::Tensor<bool> &);
template txeo::Tensor<long> operator-(const txeo::Tensor<long> &, const txeo::Tensor<long> &);
template txeo::Tensor<long long> operator-(const txeo::Tensor<long long> &,
                                           const txeo::Tensor<long long> &);
template txeo::Tensor<float> operator-(const txeo::Tensor<float> &, const txeo::Tensor<float> &);
template txeo::Tensor<double> operator-(const txeo::Tensor<double> &, const txeo::Tensor<double> &);

template txeo::Tensor<short> operator-(const txeo::Tensor<short> &, const short &);
template txeo::Tensor<int> operator-(const txeo::Tensor<int> &, const int &);
template txeo::Tensor<bool> operator-(const txeo::Tensor<bool> &, const bool &);
template txeo::Tensor<long> operator-(const txeo::Tensor<long> &, const long &);
template txeo::Tensor<long long> operator-(const txeo::Tensor<long long> &, const long long &);
template txeo::Tensor<float> operator-(const txeo::Tensor<float> &, const float &);
template txeo::Tensor<double> operator-(const txeo::Tensor<double> &, const double &);

template txeo::Tensor<short> operator-(const short &, const txeo::Tensor<short> &);
template txeo::Tensor<int> operator-(const int &, const txeo::Tensor<int> &);
template txeo::Tensor<bool> operator-(const bool &, const txeo::Tensor<bool> &);
template txeo::Tensor<long> operator-(const long &, const txeo::Tensor<long> &);
template txeo::Tensor<long long> operator-(const long long &, const txeo::Tensor<long long> &);
template txeo::Tensor<float> operator-(const float &, const txeo::Tensor<float> &);
template txeo::Tensor<double> operator-(const double &, const txeo::Tensor<double> &);

template txeo::Tensor<short> operator*(const txeo::Tensor<short> &, const short &);
template txeo::Tensor<int> operator*(const txeo::Tensor<int> &, const int &);
template txeo::Tensor<bool> operator*(const txeo::Tensor<bool> &, const bool &);
template txeo::Tensor<long> operator*(const txeo::Tensor<long> &, const long &);
template txeo::Tensor<long long> operator*(const txeo::Tensor<long long> &, const long long &);
template txeo::Tensor<float> operator*(const txeo::Tensor<float> &, const float &);
template txeo::Tensor<double> operator*(const txeo::Tensor<double> &, const double &);

template txeo::Tensor<short> operator/(const txeo::Tensor<short> &, const short &);
template txeo::Tensor<int> operator/(const txeo::Tensor<int> &, const int &);
template txeo::Tensor<bool> operator/(const txeo::Tensor<bool> &, const bool &);
template txeo::Tensor<long> operator/(const txeo::Tensor<long> &, const long &);
template txeo::Tensor<long long> operator/(const txeo::Tensor<long long> &, const long long &);
template txeo::Tensor<float> operator/(const txeo::Tensor<float> &, const float &);
template txeo::Tensor<double> operator/(const txeo::Tensor<double> &, const double &);

template txeo::Tensor<short> operator/(const short &, const txeo::Tensor<short> &);
template txeo::Tensor<int> operator/(const int &, const txeo::Tensor<int> &);
template txeo::Tensor<bool> operator/(const bool &, const txeo::Tensor<bool> &);
template txeo::Tensor<long> operator/(const long &, const txeo::Tensor<long> &);
template txeo::Tensor<long long> operator/(const long long &, const txeo::Tensor<long long> &);
template txeo::Tensor<float> operator/(const float &, const txeo::Tensor<float> &);
template txeo::Tensor<double> operator/(const double &, const txeo::Tensor<double> &);

} // namespace txeo
