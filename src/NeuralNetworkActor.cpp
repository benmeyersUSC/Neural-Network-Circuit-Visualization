//
// Created by Ben Meyers on 2/18/26.
//

#include "NeuralNetworkActor.h"
#include "DrawComponent.h"
#include "Math.h"
#include <algorithm>
#include <cmath>


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
        int inCount  = static_cast<int>(W.Cols());  // W is [out x in], so Cols = input neuron count
        int outCount = static_cast<int>(W.Rows());  // Rows = output neuron count

        // find the largest absolute weight in this layer so we can normalize against it
        float maxMag = 0.0f;
        for (int i = 0; i < inCount; ++i)
            for (int j = 0; j < outCount; ++j)
                maxMag = std::max(maxMag, std::fabs(W.at(static_cast<size_t>(j), static_cast<size_t>(i))));

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
                float normalized = (maxMag > 0.0f) ? std::fabs(w) / maxMag : 0.0f;
                auto alpha = static_cast<Uint8>(Math::Clamp(normalized * 255.0f, 30.0f, 255.0f));

                mDraw->AddLine(src.x, src.y, dst.x, dst.y, 255, 0, 0, alpha);
            }
        }
    }
}

void NeuralNetworkActor::DrawNeurons(size_t totalCols, const std::vector<Layer>& layers, const std::vector<int>& neuronCounts, float colStep, float ox, float oy, float radius) {
    for (int c = 0; c < totalCols; ++c)
    {
        int nCount = neuronCounts[c];

        // Activation info for this column
        // col 0 is the input layer — no activation
        bool isInput = (c == 0);
        Activation act{};
        if (!isInput)
            act = layers[c - 1].activation;

        // Circle color by activation
        Uint8 cr, cg, cb;
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
        const char* label = nullptr;
        if (!isInput) {
            switch (act) {
                case Activation::Sigmoid: label = "s"; break;
                case Activation::ReLU:    label = "R"; break;
                case Activation::Softmax: label = "S"; break;
            }
        }

        for (int n = 0; n < nCount; ++n)
        {
            Vector2 pos = NeuronPos(c, n, nCount, colStep, ox, oy);

            mDraw->AddFilledCircle(pos.x, pos.y, radius, cr, cg, cb, 255);

            if (label)
            {
                // Scale text to fit inside circle; SDL debug chars are 8px wide/tall
                float textScale = Math::Max(1.0f, radius / 8.0f);
                float halfChar  = 4.0f * textScale;
                mDraw->AddText(pos.x - halfChar, pos.y - halfChar,
                               label, textScale);
            }
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

    DrawWeights(totalCols, layers, colStep, ox, oy);
    DrawNeurons(totalCols, layers, neuronCounts, colStep, ox, oy, radius);
}
