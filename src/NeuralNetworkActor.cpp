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

void NeuralNetworkActor::DrawWeights(size_t totalCols, const std::vector<Layer>& layers, float colStep, float ox, float oy) const {
    const float animP = 1.0f - mForwardTimer/ANIMATION_DURATION;
    const auto fCols = static_cast<float>(totalCols);
    // for each column (except the last, which points nowhere)
    for (int c = 0; c + 1 < totalCols; ++c)
    {
        // how far (in seconds) through [c->c+1]'s interval...
        // say animP is a 0.375 and c=1, fCols=4...animP - c/fCols = 0.375 - 0.25 = 0.125
        float phase = animP - static_cast<float>(c) / fCols;
        // scale back up by fCols so it becomes %
        // 0.125 * 4 = 50%...
        phase *= fCols;
        float colBrightness = Math::Clamp( phase, 0.0f, 1.0f);
        float swell = Math::Clamp(1.0f - std::fabs(phase - 1.0f), 0.0f, 1.0f);

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
                // swell pushes grayscale toward white at phase==1, then settles back
                auto swellGray = static_cast<Uint8>(Math::Clamp(static_cast<float>(grayscale) + swell * (255.0f - static_cast<float>(grayscale)), 0.0f, 255.0f));

                // positive weights stay grayscale; negative weights are red (bright red = large magnitude, dark red = small)
                Uint8 r = swellGray;
                Uint8 g = (w >= 0.0f) ? swellGray : 0;
                Uint8 b = (w >= 0.0f) ? swellGray : 0;

                int thickness = 2 + static_cast<int>(swell * 3.0f);
                mDraw->AddLine(src.x, src.y, dst.x, dst.y, r, g, b, static_cast<Uint8>(255.0f * colBrightness), thickness);
            }
        }
    }
}

void NeuralNetworkActor::DrawNeurons(size_t totalCols, const std::vector<Layer>& layers, const std::vector<int>& neuronCounts,
                                       float colStep, float ox, float oy, float radius) {
    const float animP = 1.0f - mForwardTimer/ANIMATION_DURATION;
    const auto fCols = static_cast<float>(totalCols);

    // for each column
    for (int c = 0; c < totalCols; ++c)
    {
        float phase = animP - static_cast<float>(c-1) / fCols;
        phase *= fCols;
        float colBrightness = Math::Clamp( phase, 0.0f, 1.0f);
        // triangle peaked at phase==1: grows in, peaks, shrinks back to base radius
        float swell = Math::Clamp(1.0f - std::fabs(phase - 1.0f), 0.0f, 1.0f);
        float colRadius = radius + (radius * 0.5f) * swell;

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
                maxAct = std::max(maxAct, std::fabs(mLastActivation[c].at(static_cast<size_t>(k), 0)));
            alphaScale = (maxAct > 0.0f) ? 1.0f / maxAct : 0.0f;
        }

        // do this here because all activations are same in a layer
        const auto len = static_cast<float>(label.size());
        float textScale = colRadius * 2.0f / (len * Game::CHAR_PIXELS);
        float halfChar  = Game::HALF_CHAR_PIXELS * textScale;
        for (int n = 0; n < nCount; ++n)
        {
            Vector2 pos = NeuronPos(c, n, nCount, colStep, ox, oy);

            float val  = std::fabs(mLastActivation[c].at(static_cast<size_t>(n), 0));
            auto  alpha = static_cast<Uint8>(Math::Clamp(val * alphaScale * 255.0f, 0.0f, 255.0f));

            mDraw->AddFilledCircle(pos.x, pos.y, colRadius, cr, cg, cb, static_cast<Uint8>(alpha * colBrightness));
            mDraw->AddText(pos.x - halfChar * len, pos.y - halfChar, label, textScale);
        }
    }
}

void NeuralNetworkActor::StartGraphicForward() {
    mForwardTimer = ANIMATION_DURATION;
    mBackwardTimer = 0.0f;
    auto inp = DynamicMatrix(mNN.Layers()[0].weights.Cols(), 1);
    float x = 1.0f;
    inp = inp.Apply([&x](float){ x += 1.2f; return x; });
    mLastActivation = mNN.ForwardAll(inp);
}

void NeuralNetworkActor::StartGraphicTrain() {
    // hardcoded input: incrementing values
    auto inp = DynamicMatrix(mNN.Layers()[0].weights.Cols(), 1);
    float x = 1.0f;
    inp = inp.Apply([&x](float){ x += 1.2f; return x; });

    // hardcoded target: one-hot [1, 0, 0, ...]
    size_t outSize = mNN.Layers().back().weights.Rows();
    DynamicMatrix target(outSize, 1);
    target.at(0, 0) = 1.0f;

    TrainSnapshot snap = mNN.TrainStep(inp, target, 0.075f, 0.005f);
    mLastActivation  = snap.activations;
    mLastDeltas      = snap.deltas;
    mLastWeightGrads = snap.weightGradients;

    SDL_Log("loss: %.4f", snap.loss);

    mForwardTimer  = ANIMATION_DURATION;
    mBackwardTimer = 0.0f;
}

void NeuralNetworkActor::DrawBackwardWeights(size_t totalCols, float colStep, float ox, float oy) const {
    if (mLastWeightGrads.empty()) return;
    const float animP = 1.0f - mBackwardTimer / ANIMATION_DURATION;
    const float fCols = static_cast<float>(totalCols);

    for (int c = 0; c + 1 < totalCols; ++c) {
        // reversed: last weight group (rightmost) fires first
        int rev = static_cast<int>(totalCols) - 2 - c;
        float phase = animP - static_cast<float>(rev) / fCols;
        phase *= fCols;
        float colBrightness = Math::Clamp(phase, 0.0f, 1.0f);
        float swell = Math::Clamp(1.0f - std::fabs(phase - 1.0f), 0.0f, 1.0f);

        const DynamicMatrix& dW = mLastWeightGrads[c];
        int inCount  = static_cast<int>(dW.Cols());
        int outCount = static_cast<int>(dW.Rows());

        float maxMag = 0.0f;
        for (int i = 0; i < inCount; ++i)
            for (int j = 0; j < outCount; ++j)
                maxMag = std::max(maxMag, std::fabs(dW.at(j, i)));
        if (maxMag == 0.0f) continue;

        for (int i = 0; i < inCount; ++i) {
            Vector2 src = NeuronPos(c, i, inCount, colStep, ox, oy);
            for (int j = 0; j < outCount; ++j) {
                Vector2 dst = NeuronPos(c + 1, j, outCount, colStep, ox, oy);
                float normalized = std::fabs(dW.at(j, i)) / maxMag;
                // red-orange base, swell toward white
                float baseR = Math::Clamp(normalized * 255.0f, 30.0f, 255.0f);
                auto swellR = static_cast<Uint8>(baseR + swell * (255.0f - baseR));
                auto swellG = static_cast<Uint8>(80.0f  + swell * (255.0f - 80.0f));
                auto swellB = static_cast<Uint8>(50.0f  + swell * (255.0f - 50.0f));
                int thickness = 1 + static_cast<int>(swell * 3.0f);
                mDraw->AddLine(src.x, src.y, dst.x, dst.y, swellR, swellG, swellB,
                               static_cast<Uint8>(255.0f * colBrightness), thickness);
            }
        }
    }
}

void NeuralNetworkActor::DrawBackwardNeurons(size_t totalCols, const std::vector<int>& neuronCounts,
                                              float colStep, float ox, float oy, float radius) const {
    if (mLastDeltas.empty()) return;
    const float animP = 1.0f - mBackwardTimer / ANIMATION_DURATION;
    const float fCols = static_cast<float>(totalCols);

    for (int c = 0; c < totalCols; ++c) {
        // reversed with same -1 offset as forward neurons
        int rev = static_cast<int>(totalCols) - 1 - c;
        float phase = animP - static_cast<float>(rev - 1) / fCols;
        phase *= fCols;
        float colBrightness = Math::Clamp(phase, 0.0f, 1.0f);
        float swell = Math::Clamp(1.0f - std::fabs(phase - 1.0f), 0.0f, 1.0f);
        float colRadius = radius + (radius * 0.5f) * swell;

        int nCount = neuronCounts[c];

        // deltas[0] = layer1 delta (column 1), deltas[L-1] = output delta
        // for column 0 (input), no delta — draw at uniform brightness
        if (c == 0) {
            for (int n = 0; n < nCount; ++n) {
                Vector2 pos = NeuronPos(0, n, nCount, colStep, ox, oy);
                mDraw->AddFilledCircle(pos.x, pos.y, colRadius, 255, 80, 80,
                                       static_cast<Uint8>(200.0f * colBrightness));
            }
            continue;
        }

        const DynamicMatrix& delta = mLastDeltas[c - 1];
        float maxMag = 0.0f;
        for (int k = 0; k < nCount; ++k)
            maxMag = std::max(maxMag, std::fabs(delta.at(k, 0)));
        float alphaScale = (maxMag > 0.0f) ? 1.0f / maxMag : 0.0f;

        for (int n = 0; n < nCount; ++n) {
            Vector2 pos = NeuronPos(c, n, nCount, colStep, ox, oy);
            float val = std::fabs(delta.at(n, 0));
            auto alpha = static_cast<Uint8>(Math::Clamp(val * alphaScale * 255.0f, 0.0f, 255.0f));
            mDraw->AddFilledCircle(pos.x, pos.y, colRadius, 255, 80, 80,
                                   static_cast<Uint8>(alpha * colBrightness));
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

    if (mBackwardTimer > 0.0f) {
        DrawBackwardWeights(totalCols, colStep, ox, oy);
        DrawBackwardNeurons(totalCols, neuronCounts, colStep, ox, oy, radius);
    }
}

void NeuralNetworkActor::HandleUpdate(float deltaTime) {
    Actor::HandleUpdate(deltaTime);

    if (mForwardTimer > 0.0f) {
        mForwardTimer -= deltaTime;
        if (mForwardTimer <= 0.0f) {
            mForwardTimer = 0.0f;
            // forward finished — start backward if training
            if (mIsTraining) mBackwardTimer = ANIMATION_DURATION;
        }
    } else if (mBackwardTimer > 0.0f) {
        mBackwardTimer -= deltaTime;
        if (mBackwardTimer <= 0.0f) {
            mBackwardTimer = 0.0f;
            // backward finished — kick off next training step
            if (mIsTraining) StartGraphicTrain();
        }
    }
}

void NeuralNetworkActor::HandleInput(const bool keys[], SDL_MouseButtonFlags mouseButtons, const Vector2 &posMouse) {
    Actor::HandleInput(keys, mouseButtons, posMouse);
    Game::LeadingEdge(keys[SDL_SCANCODE_R], mLastR, mRFunc);
    Game::LeadingEdge(keys[SDL_SCANCODE_T], mLastT, mTFunc);
}
