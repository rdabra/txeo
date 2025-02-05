#ifndef TENSOR_SHAPE_PRIVATE_H
#define TENSOR_SHAPE_PRIVATE_H
#pragma once

#include "txeo/TensorShape.h"
#include <tensorflow/core/framework/tensor_shape.h>

struct txeo::TensorShape::Impl {
    std::unique_ptr<tensorflow::TensorShape> tf_shape{nullptr};
};

#endif