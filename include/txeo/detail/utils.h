#ifndef TXEO_UTILS_H
#define TXEO_UTILS_H
#pragma once

#include <cstdint> // for int64_t
#include <cstdlib> // for size_t, abs
#include <limits>  // for numeric_limits
#include <string>  // for string
#include <tensorflow/core/framework/tensor.h>
#include <tensorflow/core/framework/tensor_shape.h>
#include <tensorflow/core/framework/types.h>    // for DataTypeToEnum, Enum...
#include <tensorflow/core/framework/types.pb.h> // for DataType
#include <type_traits>                          // for is_floating_point_v
#include <vector>                               // for vector

#include "txeo/TensorShape.h" // for TensorShape

namespace tensorflow {
class TensorShape;
class TensorShapeProto;
} // namespace tensorflow

namespace tf = tensorflow;

namespace txeo::detail {

/**
 * @brief Gets the correspondent TensorFlow type from the cpp type
 *
 * @tparam T cpp type
 * @return tf::DataType TensorFlow type
 */
template <typename T>
constexpr tf::DataType get_tf_dtype() {
  static_assert(tf::DataTypeToEnum<T>::value != tf::DT_INVALID,
                "Unsupported C++ type for TensorFlow mapping");
  return tf::DataTypeToEnum<T>::value;
}

/**
 * @brief Specifies the cpp type from the TensorFlow type
 *
 * @tparam T TensorFlow type
 */
template <tf::DataType T>
using cpp_type = typename tf::EnumToDataType<T>::Type;

int64_t to_int64(const size_t &val);

size_t to_size_t(const int64_t &val);

std::vector<size_t> to_size_t(const std::vector<int64_t> &vec);

std::vector<int64_t> to_int64(const std::vector<size_t> &vec);

int to_int(const size_t &val);

int to_int(const int64_t &val);

std::string format(const double &a, int precision);

txeo::TensorShape to_txeo_tensor_shape(const tf::TensorShape &shape);

txeo::TensorShape proto_to_txeo_tensor_shape(const tf::TensorShapeProto &shape);

std::vector<size_t> calc_stride(const tf::TensorShape &shape);

template <typename T>
bool is_zero(T value) {
  if constexpr (std::is_floating_point_v<T>)
    return std::abs(value) < std::numeric_limits<T>::epsilon();

  return value == 0;
}

bool is_numeric(const std::string &word);

std::string current_time();

} // namespace txeo::detail

#endif