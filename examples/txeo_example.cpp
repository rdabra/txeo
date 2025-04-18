
#include "txeo/DataTable.h"
#include "txeo/Loss.h"
#include "txeo/Matrix.h"
#include "txeo/MatrixIO.h"
#include "txeo/OlsGDTrainer.h"
#include "txeo/Tensor.h"
#include "txeo/TensorAgg.h"
#include "txeo/TensorFunc.h"
#include "txeo/TensorIO.h"
#include "txeo/TensorOp.h"
#include "txeo/TensorPart.h"
#include "txeo/types.h"
#include <cmath>

#include <iostream>
#include <ostream>
#include <tensorflow/cc/client/client_session.h>
#include <tensorflow/cc/ops/math_ops.h>
#include <tensorflow/cc/ops/standard_ops.h>
#include <tensorflow/core/framework/tensor.h>

namespace tf = tensorflow;
using namespace tensorflow::ops;

class Foo {
  private:
    const int *_num;

  public:
    Foo(const int &num) : _num{&num} {};

    [[nodiscard]] const int &num() const { return *_num; }
};

int main() {

  txeo::Matrix<double> data(4, 2, {1, 3, 2, 6, 3, 9, 5, 15});
  txeo::OlsGDTrainer<double> trainer{txeo::DataTable<double>{std::move(data), {1}}};
  trainer.enable_feature_norm(txeo::NormalizationType::MIN_MAX);
  trainer.enable_variable_lr();
  trainer.fit(100, txeo::LossFunc::MAE, 5);

  txeo::Matrix<double> x(1, 1, {4});

  std::cout << trainer.predict(x) << std::endl;

  trainer.disable_feature_norm();
  trainer.fit(100, txeo::LossFunc::MAE, 5);

  std::cout << trainer.predict(x) << std::endl;

  std::cout << "Min loss: " << trainer.min_loss() << std::endl;

  // txeo::Matrix<double> data(5, 2, {1, 5, 2, 6, 3, 7, 5, 8, 10, 10});

  // auto funcs = txeo::TensorFunc<double>::normalize_by(data, 0, txeo::NormalizationType::MIN_MAX);

  // std::cout << "Size: " << funcs.size() << std::endl;

  // std::cout << funcs[0](4) << std::endl;
  // std::cout << funcs[1](9) << std::endl;

  // txeo::Matrix<double> data(100, 5);
  // txeo::DataTable<double> dt(data, {0, 1, 2}, {3, 4}, 30);

  // std::cout << dt.x_eval()->row_size() << std::endl;

  // txeo::Tensor<double> t;

  // std::cout << t.shape().calculate_capacity() << std::endl;

  // txeo::Matrix<double> X({{1.0}, {2.0}, {3.0}}); // 3x1
  // txeo::Matrix<double> y({{3.0}, {5.0}, {7.0}}); // 3x1

  // txeo::OlsGDTrainer<double> trainer(X, y);
  // trainer.set_tolerance(0.0001);
  // trainer.enable_variable_lr();

  // // Train with early stopping
  // trainer.fit(1000, txeo::LossFunc::MSE, 10);

  // if (trainer.is_converged()) {
  //   auto weights = trainer.weight_bias();
  //   std::cout << "Model: y = " << weights(0, 0) << "x + " << weights(1, 0) << std::endl;

  //   // Make prediction
  //   txeo::Matrix<double> test_input(1, 1, {4.0});
  //   auto prediction = trainer.predict(test_input);
  //   std::cout << "Prediction for x=4: " << prediction(0, 0) << std::endl;
  // }

  // Data downloaded at
  // https://media.geeksforgeeks.org/wp-content/uploads/20240319120216/housing.csv
  // auto train_data =
  //     txeo::MatrixIO::one_hot_encode_text_file("/home/roberto/Downloads/housing.csv", ',', true,
  //                                              "/home/roberto/Downloads/housing_one_hot.csv");

  // auto x_train = txeo::TensorPart<double>::sub_matrix_cols_exclude(
  //     train_data.read_text_file<double>(true), {8});
  // auto y_train =
  //     txeo::TensorPart<double>::sub_matrix_cols(train_data.read_text_file<double>(true), {8});

  // x_train.normalize_columns(txeo::NormalizationType::MIN_MAX);

  // std::cout << "X: " << x_train.shape() << std::endl;
  // std::cout << "Y: " << y_train.shape() << std::endl;

  // txeo::OlsGDTrainer<double> ols{x_train, y_train};

  // ols.enable_variable_lr();
  // ols.fit(100, txeo::LossFunc::MAE);

  // std::cout << "Weight-Bias: " << ols.weight_bias() << std::endl;
  // std::cout << "Minimun loss: " << ols.min_loss() << std::endl;
  // std::cout << "min Y:" << txeo::TensorAgg<double>::reduce_min(y_train, {0})() << std::endl;
}
