//
// Created by Ben Meyers on 2/18/26.
//

#include "NeuralNetworkActor.h"
#include "DrawComponent.h"
#include "Math.h"
#include <algorithm>
#include <cmath>
#include "Game.h"

NeuralNetworkActor::NeuralNetworkActor():mWidth(0.0f), mHeight(0.0f) {
    mDraw = CreateComponent<DrawComponent>();
}


Vector2 NeuralNetworkActor::NeuronPos(int col, int neuronIdx, int neuronCount,
                                      float colStep, float originX, float originY) const
{
    // +1 because the height includes half neurons on top/bottom
    float rowStep = mHeight / static_cast<float>(neuronCount + 1);

    // simple x and y shifts
    float x = originX + static_cast<float>(col) * colStep;
    float y = originY + static_cast<float>(neuronIdx + 1) * rowStep;
    return {x, y};
}

float NeuralNetworkActor::NeuronRadius(float colStep, float minRowStep)
{
    // total magic number...max for it would be 0.5f
    // because then these two half radii would kiss
    return Math::Min(colStep, minRowStep) * 0.36f;
}

void NeuralNetworkActor::SetNN(NeuralNetwork nn) {
    mNN = std::move(nn);
}

void NeuralNetworkActor::DrawWeights(size_t totalCols, const std::vector<Layer>& layers, float colStep, float ox, float oy) {
    // for each column (except the last, which points nowhere)
    for (int c = 0; c + 1 < totalCols; ++c)
    {
        // get the weight matrix that transforms to next column
        const DynamicMatrix& W = layers[c].weights;
        // weight columns is num input rows
        int inCount  = static_cast<int>(W.Cols());
        // weight rows is num output rows
        int outCount = static_cast<int>(W.Rows());

        // find the largest absolute weight in this layer so we can normalize against it
        float maxMag = 0.0f;
        for (int i = 0; i < inCount; ++i) {
            for (int j = 0; j < outCount; ++j) {
                maxMag = std::max(maxMag, std::fabs(W.at(static_cast<size_t>(j), static_cast<size_t>(i))));
            }
        }

        // for each FROM
        for (int i = 0; i < inCount; ++i)
        {
            // get its neuron position
            Vector2 src = NeuronPos(c, i, inCount, colStep, ox, oy);
            // and draw lines for each TO neuron it points to
            for (int j = 0; j < outCount; ++j)
            {
                // TO neuron position
                Vector2 dst = NeuronPos(c + 1, j, outCount, colStep, ox, oy);

                float w = W.at(static_cast<size_t>(j), static_cast<size_t>(i));
                // normalize so the strongest weight in this layer maps to alpha 255
                float normalized = std::fabs(w) / maxMag;
                auto grayscale = static_cast<Uint8>(Math::Clamp(normalized * 255.0f, 30.0f, 255.0f));

                mDraw->AddLine(src.x, src.y, dst.x, dst.y, grayscale, grayscale, grayscale, 255, 3);
            }
        }
    }
}

void NeuralNetworkActor::DrawNeurons(size_t totalCols, const std::vector<Layer>& layers, const std::vector<int>& neuronCounts,
                                      const std::vector<DynamicMatrix>& activations, float colStep, float ox, float oy, float radius) {
    // for each column
    for (int c = 0; c < totalCols; ++c)
    {
        // how many neurons
        int nCount = neuronCounts[c];

        // activation func
        bool isInput = c == 0;
        Activation act{};
        if (!isInput) {
            // -1 because the layer is the weight-bias-activation that comes to this (the i-th) column
            act = layers[c - 1].activation;
        }

        // color each neuron by activation type
        // let's alpha scale them by their value in a forward()....
        Uint8 cr = 0, cg = 0, cb = 0;
        if (isInput) {
            cr = 200; cg = 200; cb = 200;   // white-gray
        } else {
            switch (act) {
                case Activation::Sigmoid:
                    cr = 80;  cg = 140; cb = 255; break;  // blue
                case Activation::ReLU:
                    cr = 80;  cg = 220; cb = 100; break;  // green
                case Activation::Softmax:
                    cr = 255; cg = 160; cb = 50;  break;  // orange
            }
        }

        // Label character
        std::string label;
        if (!isInput) {
            switch (act) {
                case Activation::Sigmoid: label += "sig"; break;
                case Activation::ReLU:    label += "ReLU"; break;
                case Activation::Softmax: label += "SM"; break;
            }
        }
        // relu is unbounded so normalize by the layer's max activation;
        // sigmoid/softmax are already in [0,1] so scale = 1.0
        float alphaScale = 1.0f;
        if (isInput || act == Activation::ReLU) {
            float maxAct = 0.0f;
            for (int k = 0; k < nCount; ++k)
                maxAct = std::max(maxAct, std::fabs(activations[c].at(static_cast<size_t>(k), 0)));
            alphaScale = (maxAct > 0.0f) ? 1.0f / maxAct : 0.0f;
        }

        // do this here because all activations are same in a layer
        const auto len = static_cast<float>(label.size());
        float textScale = radius * 2.0f / (len * Game::CHAR_PIXELS);
        float halfChar  = Game::HALF_CHAR_PIXELS * textScale;
        for (int n = 0; n < nCount; ++n)
        {
            Vector2 pos = NeuronPos(c, n, nCount, colStep, ox, oy);

            float val  = std::fabs(activations[c].at(static_cast<size_t>(n), 0));
            auto  alpha = static_cast<Uint8>(Math::Clamp(val * alphaScale * 255.0f, 0.0f, 255.0f));

            mDraw->AddFilledCircle(pos.x, pos.y, radius, cr, cg, cb, alpha);
            mDraw->AddText(pos.x - halfChar * len, pos.y - halfChar, label, textScale);
        }
    }
}

void NeuralNetworkActor::HandleRender()
{
    const auto& layers = mNN.Layers();
    if (layers.empty()) return;

    // get actual rectangle origin coordinates
    const Vector2& center = GetTransform().GetPosition();
    const float ox = center.x - mWidth  / 2.0f;
    const float oy = center.y - mHeight / 2.0f;

    // +1 for input column (which is not actually a Layer, but which we do render)
    const int totalCols = static_cast<int>(layers.size()) + 1;
    // -1 because we have N columns so N-1 gaps
    const float colStep = mWidth / static_cast<float>(totalCols - 1);

    // neuron counts for each column
    std::vector<int> neuronCounts(totalCols);
    neuronCounts[0] = static_cast<int>(layers[0].weights.Cols());
    for (int c = 1; c < totalCols; ++c) {
        neuronCounts[c] = static_cast<int>(layers[c - 1].weights.Rows());
    }

    // radius is determined by the smallest row gap
    int maxNeurons = *std::max_element(neuronCounts.begin(), neuronCounts.end());
    float minRowStep = mHeight / static_cast<float>(maxNeurons + 1);
    float radius = NeuronRadius(colStep, minRowStep);

    // if no input has been set, default to a ones vector so the network is visible
    auto inp = DynamicMatrix(static_cast<size_t>(neuronCounts[0]), 1);
    float x = 1.0f;
    auto bsfn = [&x](float f){
        x += 1.2f;
        return x;
    };
    inp = inp.Apply(bsfn);

    auto activations = mNN.ForwardAll(inp);

    DrawWeights(totalCols, layers, colStep, ox, oy);
    DrawNeurons(totalCols, layers, neuronCounts, activations, colStep, ox, oy, radius);
}
