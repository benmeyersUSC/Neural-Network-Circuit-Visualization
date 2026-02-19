//
// Created by Ben Meyers on 2/18/26.
//

#include "NeuralNetwork.h"

#include <cmath>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>


// simple implementations
static float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }
static float relu(float x)    { return x > 0.0f ? x : 0.0f; }
static DynamicMatrix softmax(const DynamicMatrix& z) {
    // get maximum value
    float maxVal = z.at(0, 0);
    for (size_t i = 1; i < z.Rows(); i++)
        maxVal = std::max(maxVal, z.at(i, 0));

    // matrix output is column vector
    DynamicMatrix out(z.Rows(), 1);
    float sum = 0.0f;
    for (size_t i = 0; i < z.Rows(); i++) {
        // summation of e^[value - maxVal]
        out.at(i, 0) = std::exp(z.at(i, 0) - maxVal);
        sum += out.at(i, 0);
    }
    // divide each term by this sum
    for (size_t i = 0; i < out.Rows(); i++)
        out.at(i, 0) /= sum;
    return out;
}


DynamicMatrix Layer::forward(const DynamicMatrix& input) const {
    // use our overloads!
    DynamicMatrix z = weights * input + biases;

    // if activation is softmax, call special function
    if (activation == Activation::Softmax) {
        return softmax(z);
    }

    // grab activation function
    const auto fn = activation == Activation::Sigmoid ? sigmoid : relu;
    // and apply it
    return z.Apply(fn);
}


// parse an activation token in config file
static Activation ParseActivation(const std::string& s) {
    if (s == "input")   return Activation::Input;
    if (s == "sigmoid") return Activation::Sigmoid;
    if (s == "ReLU")    return Activation::ReLU;
    if (s == "softmax") return Activation::Softmax;
    throw std::runtime_error("Unknown activation: \"" + s + "\"");
}

struct LayerSpec {
    size_t     neurons;
    Activation activation;
};

// will take a line string and parse into activation tokens
static LayerSpec ParseLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    // chew through | chars, pushing back the tokens
    while (std::getline(ss, token, '|')) {
        if (!token.empty()) tokens.push_back(token);
    }

    if (tokens.empty())
        throw std::runtime_error("Empty layer line in config");

    // the layer spec becomes the number of tokens and the first activation
    // this means you can write like:
    //  "|sigmoid|*|*|*|*|*|"
    return { tokens.size(), ParseActivation(tokens[0]) };
}

void NeuralNetwork::FromConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open config file: " + path);

    std::vector<LayerSpec> specs;
    std::string line;
    while (std::getline(file, line)) {
        // skip blank lines and those without |
        if (line.find('|') == std::string::npos) continue;
        specs.push_back(ParseLine(line));
    }

    if (specs.size() < 2)
        throw std::runtime_error("Config needs at least 2 layers (input + one more)");

    std::mt19937 rng(std::random_device{}());

    mLayers.clear();

    // loop through layer specs
    // specs[0] = input layer (defines input size only, no weight matrix)
    // specs[1...n] = actual layers with weights
    for (size_t i = 1; i < specs.size(); i++) {
        size_t inSize  = specs[i - 1].neurons;
        size_t outSize = specs[i].neurons;

        // Xavier uniform: limit = sqrt(6 / (fan_in + fan_out))
        float limit = std::sqrt(6.0f / static_cast<float>(inSize + outSize));
        std::uniform_real_distribution<float> dist(-limit, limit);

        // create weight matrix based on layer specs
        DynamicMatrix weights(outSize, inSize);
        for (size_t r = 0; r < outSize; r++)
            for (size_t c = 0; c < inSize; c++)
                weights.at(r, c) = dist(rng);

        // bias matrix for this layer
        DynamicMatrix biases(outSize, 1);

        // use move to define .weights and .bias, and pass activation function
        mLayers.push_back({ std::move(weights), std::move(biases), specs[i].activation });
    }
}

// run a forward pass and return a vector of all activations!
std::vector<DynamicMatrix> NeuralNetwork::ForwardAll(const DynamicMatrix& input) const {
    std::vector<DynamicMatrix> acts;
    acts.reserve(mLayers.size() + 1);
    acts.push_back(input);
    for (const auto& layer : mLayers)
        acts.push_back(layer.forward(acts.back()));
    return acts;
}

DynamicMatrix NeuralNetwork::forward(const DynamicMatrix& input) const {
    if (mLayers.empty())
        throw std::runtime_error("NeuralNetwork has no layers");

    DynamicMatrix current = input;
    for (const auto& layer : mLayers)
        current = layer.forward(current);
    return current;
}

// ---- activation derivatives ----
// sigmoid'(a) = a*(1-a)  — takes post-activation value
static float sigmoidPrime(float a) { return a * (1.0f - a); }
// relu'(z) = z > 0 ? 1 : 0  — takes pre-activation value
static float reluPrime(float z)    { return z > 0.0f ? 1.0f : 0.0f; }
// softmax jacobian applied to error vector: a ⊙ (e - <a,e>)
static DynamicMatrix softmaxDelta(const DynamicMatrix& a, const DynamicMatrix& e) {
    float dot = 0.0f;
    for (size_t i = 0; i < a.Rows(); i++) dot += a.at(i, 0) * e.at(i, 0);
    DynamicMatrix d(a.Rows(), 1);
    for (size_t i = 0; i < a.Rows(); i++) d.at(i, 0) = a.at(i, 0) * (e.at(i, 0) - dot);
    return d;
}

TrainSnapshot NeuralNetwork::TrainStep(const DynamicMatrix& input,
                                       const DynamicMatrix& target,
                                       float lr, float l1)
{
    const size_t L = mLayers.size();

    // === FORWARD — capture z (pre-activation) and a (post-activation) at every layer ===
    std::vector<DynamicMatrix> Z;  Z.reserve(L);
    std::vector<DynamicMatrix> A;  A.reserve(L + 1);
    A.push_back(input);
    for (const auto& layer : mLayers) {
        DynamicMatrix z = layer.weights * A.back() + layer.biases;
        Z.push_back(z);
        if (layer.activation == Activation::Softmax)
            A.push_back(softmax(z));
        else
            A.push_back(z.Apply(layer.activation == Activation::Sigmoid ? sigmoid : relu));
    }

    // === LOSS (cross-entropy: -sum(y * log(a))) ===
    float loss = 0.0f;
    for (size_t i = 0; i < A.back().Rows(); i++) {
        float a = std::max(A.back().at(i, 0), 1e-7f);  // clamp for log stability
        loss -= target.at(i, 0) * std::log(a);
    }

    // === BACKWARD ===
    std::vector deltas(L, DynamicMatrix(1, 1));
    std::vector dW(L,     DynamicMatrix(1, 1));
    std::vector dB(L,     DynamicMatrix(1, 1));

    // output layer: cross-entropy gradient w.r.t. softmax/sigmoid pre-activation = a - y
    // (softmax+CE and sigmoid+CE both simplify to this — the activation derivative cancels)
    deltas[L-1] = A[L] - target;
    dW[L-1] = deltas[L-1] * A[L-1].Transpose();
    dB[L-1] = deltas[L-1];

    // hidden layers — walk backward, apply activation derivative here
    for (int l = static_cast<int>(L) - 2; l >= 0; --l) {
        DynamicMatrix err = mLayers[l+1].weights.Transpose() * deltas[l+1];
        switch (mLayers[l].activation) {
            case Activation::Sigmoid:
                deltas[l] = err.HadamardProduct(A[l+1].Apply(sigmoidPrime)); break;
            case Activation::ReLU:
                deltas[l] = err.HadamardProduct(Z[l].Apply(reluPrime));      break;
            case Activation::Softmax:
                deltas[l] = softmaxDelta(A[l+1], err);                       break;
        }
        dW[l] = deltas[l] * A[l].Transpose();
        dB[l] = deltas[l];
    }

    // === L1 SPARSITY: add lambda*|W| to loss, lambda*sign(W) to gradients ===
    if (l1 > 0.0f) {
        for (size_t l = 0; l < L; l++) {
            for (size_t r = 0; r < dW[l].Rows(); r++) {
                for (size_t c = 0; c < dW[l].Cols(); c++) {
                    float w = mLayers[l].weights.at(r, c);
                    loss += l1 * std::fabs(w);
                    dW[l].at(r, c) += l1 * (w > 0.0f ? 1.0f : (w < 0.0f ? -1.0f : 0.0f));
                }
            }
        }
    }

    // === UPDATE WEIGHTS ===
    for (size_t l = 0; l < L; l++) {
        mLayers[l].weights = mLayers[l].weights + dW[l] * (-lr);
        mLayers[l].biases  = mLayers[l].biases  + dB[l] * (-lr);
    }

    return { A, deltas, dW, loss };
}

void NeuralNetwork::operator<<(std::ostream &os) const {
    for (const auto& l : mLayers) {
        os << l.weights.Rows() << "x" << l.weights.Cols() << "(" << ActivationName(l.activation) << ")\n";
    }
}
