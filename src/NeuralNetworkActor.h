//
// Created by Ben Meyers on 2/18/26.
//

#ifndef NEURAL_NETWORK_ACTOR_H
#define NEURAL_NETWORK_ACTOR_H

#include "Actor.h"
#include "NeuralNetwork.h"

class DrawComponent;

class NeuralNetworkActor : public Actor {
    static constexpr float ANIMATION_DURATION = 1.0f;
public:
    NeuralNetworkActor();
    NeuralNetwork& GetNN() {return mNN;}
    void SetWidth(float w){mWidth = w;}
    void SetHeight(float h){mHeight = h;}
    void StartGraphicForward();
    void StartGraphicTrain();

protected:
    void HandleRender() override;
    void HandleUpdate(float deltaTime) override;
    void HandleInput(const bool keys[], SDL_MouseButtonFlags mouseButtons, const Vector2 &posMouse) override;

private:
    NeuralNetwork  mNN;
    float          mWidth;
    float          mHeight;
    DrawComponent* mDraw = nullptr;

    float mForwardTimer  = 0.0f;
    float mBackwardTimer = 0.0f;
    bool  mIsTraining    = false;

    std::vector<DynamicMatrix> mLastActivation;  // a at each column, size = totalCols
    std::vector<DynamicMatrix> mLastDeltas;       // δ at each layer, size = L (col 1..L)
    std::vector<DynamicMatrix> mLastWeightGrads;  // dW per layer, size = L

    bool mLastR = false;
    std::function<void()> mRFunc = [this] { StartGraphicForward(); };
    bool mLastT = false;
    std::function<void()> mTFunc = [this] {
        mIsTraining = !mIsTraining;
        if (mIsTraining) StartGraphicTrain();
    };

    [[nodiscard]] Vector2 NeuronPos(int col, int neuronIdx, int neuronCount,
                      float colStep, float originX, float originY) const;
    static float NeuronRadius(float colStep, float minRowStep);
    void SetNN(NeuralNetwork nn);

    // forward pass draws (left → right, uses mForwardTimer)
    void DrawWeights(size_t totalCols, const std::vector<Layer>& layers, float colStep, float ox, float oy) const;
    void DrawNeurons(size_t totalCols, const std::vector<Layer>& layers, const std::vector<int>& neuronCounts, float colStep, float ox, float oy, float radius);

    // backward pass draws (right → left, uses mBackwardTimer)
    void DrawBackwardWeights(size_t totalCols, float colStep, float ox, float oy) const;
    void DrawBackwardNeurons(size_t totalCols, const std::vector<int>& neuronCounts, float colStep, float ox, float oy, float radius) const;
};

#endif // NEURAL_NETWORK_ACTOR_H
