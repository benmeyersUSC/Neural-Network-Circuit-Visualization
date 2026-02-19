//
// Created by Ben Meyers on 2/18/26.
//

#ifndef NEURAL_NETWORK_CIRCUIT_VISUALIZATION_NEURALNETWORK_H
#define NEURAL_NETWORK_CIRCUIT_VISUALIZATION_NEURALNETWORK_H

#include <vector>
#include <string>
#include "DynamicMatrix.h"

// available activation functions
enum class Activation {Sigmoid, ReLU, Softmax};

inline std::string_view ActivationName(const Activation& a) {
    return a == Activation::Sigmoid ? "Sigmoid" : a == Activation::ReLU ? "ReLU" : "Softmax";
}

// Layer wrapper for weights, bias, and activation function
struct Layer {
    DynamicMatrix weights;  // shape: [out_neurons x in_neurons]
    DynamicMatrix biases;   // shape: [out_neurons x 1]
    Activation activation;

    // compute a forward pass at this layer, taking in another matrix as input
    [[nodiscard]] DynamicMatrix forward(const DynamicMatrix& input) const;
};

class NeuralNetwork {
    // network consists of a vector of Layer objects
    std::vector<Layer> mLayers;

public:
    NeuralNetwork() = default;

    // load layers from a config file into this network
    void FromConfig(const std::string& path);

    // Run a forward pass. Input must be a column vector [input_size x 1].
    [[nodiscard]] DynamicMatrix forward(const DynamicMatrix& input) const;

    [[nodiscard]] const std::vector<Layer>& Layers() const { return mLayers; }

     void operator<<(std::ostream& os);
};

#endif //NEURAL_NETWORK_CIRCUIT_VISUALIZATION_NEURALNETWORK_H
