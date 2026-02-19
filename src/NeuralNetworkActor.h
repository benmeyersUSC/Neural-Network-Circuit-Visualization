//
// Created by Ben Meyers on 2/18/26.
//

#ifndef NEURAL_NETWORK_ACTOR_H
#define NEURAL_NETWORK_ACTOR_H

#include "Actor.h"
#include "NeuralNetwork.h"

class DrawComponent;

class NeuralNetworkActor : public Actor {
public:
    NeuralNetworkActor();
    NeuralNetwork& GetNN() {return mNN;}
    void SetWidth(float w){mWidth = w;}
    void SetHeight(float h){mHeight = h;}

protected:
    void HandleRender() override;

private:
    NeuralNetwork  mNN;
    float          mWidth;
    float          mHeight;
    DrawComponent* mDraw = nullptr;

    // screenspace center of the neuronIdx-th neuron in column col
    [[nodiscard]] Vector2 NeuronPos(int col, int neuronIdx, int neuronCount,
                      float colStep, float originX, float originY) const;

    // Radius that keeps circles from overlapping regardless of network size.
    static float NeuronRadius(float colStep, float minRowStep) ;

    void SetNN(NeuralNetwork nn);

    void DrawWeights(size_t totalCols, const std::vector<Layer>& layers, float colStep, float ox, float oy);
    void DrawNeurons(size_t totalCols, const std::vector<Layer>& layers, const std::vector<int>& neuronCounts,
                     const std::vector<DynamicMatrix>& activations, float colStep, float ox, float oy, float radius);
};

#endif // NEURAL_NETWORK_ACTOR_H