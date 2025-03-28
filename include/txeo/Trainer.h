#ifndef TRAINER_H
#define TRAINER_H
#pragma once

#include "txeo/Tensor.h"
#include "types.h"

namespace txeo {

/**
 * @class Trainer
 * @brief Abstract base class for machine learning trainers
 *
 * @tparam T Numeric type for tensor elements (e.g., float, double)
 *
 * This class provides the core interface for training machine learning models with:
 * - Training/evaluation data management
 * - Common training parameters
 * - Basic training lifecycle control
 *
 * Derived classes must implement the prediction logic and training algorithm.
 */
template <typename T>
class Trainer {
  public:
    Trainer(const Trainer &) = default;
    Trainer(Trainer &&) = default;
    Trainer &operator=(const Trainer &) = default;
    Trainer &operator=(Trainer &&) = default;
    virtual ~Trainer() = default;

    /**
     * @brief Constructs a Trainer with separate training and evaluation data
     *
     * @param x_train Training features tensor (shape: [samples, features])
     * @param y_train Training labels tensor (shape: [samples, outputs])
     * @param x_eval Evaluation features tensor (shape: [samples, features])
     * @param y_eval Evaluation labels tensor (shape: [samples, outputs])
     */
    Trainer(const txeo::Tensor<T> &x_train, const txeo::Tensor<T> &y_train,
            const txeo::Tensor<T> &x_eval, const txeo::Tensor<T> &y_eval);

    /**
     * @brief Constructs a Trainer using training data for evaluation
     *
     * @param x_train Training features tensor (shape: [samples, features])
     * @param y_train Training labels tensor (shape: [samples, outputs])
     */
    Trainer(const txeo::Tensor<T> &x_train, const txeo::Tensor<T> &y_train)
        : Trainer{x_train, y_train, x_train, y_train} {}

    /**
     * @brief Trains the model for specified number of epochs
     *
     * @param epochs Number of training iterations
     * @param metric Loss function to optimize
     */
    virtual void fit(size_t epochs, txeo::LossFunc metric);

    /**
     * @brief Trains with early stopping based on validation performance
     *
     * @param epochs Maximum number of training iterations
     * @param metric Loss function to optimize
     * @param patience Number of epochs to wait without improvement before stopping
     */
    virtual void fit(size_t epochs, txeo::LossFunc metric, size_t patience);

    /**
     * @brief Makes predictions using the trained model (pure virtual)
     *
     * @param input Input tensor for prediction (shape: [samples, features])
     * @return Prediction tensor (shape: [samples, outputs])
     *
     * @throws txeo::TrainerError if called before training
     *
     * @note Must be implemented in derived classes
     */
    virtual txeo::Tensor<T> predict(const txeo::Tensor<T> &input) = 0;

    /**
     * @brief Checks if model has been trained
     *
     * @return true if training has been completed, false otherwise
     */
    [[nodiscard]] bool is_trained() const { return _is_trained; }

  protected:
    Trainer() = default;

    bool _is_trained{false};
    bool _is_early_stop{false};
    size_t _patience{0};

    const txeo::Tensor<T> *_x_train;
    const txeo::Tensor<T> *_y_train;
    const txeo::Tensor<T> *_x_eval;
    const txeo::Tensor<T> *_y_eval;

    virtual void train(size_t epochs, txeo::LossFunc loss_func) = 0;
};

/**
 * @brief Exceptions concerning @ref txeo::OlsGDTrainer
 *
 */
class TrainerError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

} // namespace txeo

#endif