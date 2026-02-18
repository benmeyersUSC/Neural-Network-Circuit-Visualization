//
// Created by Ben Meyers on 2/15/26.
//

#include "Line.h"
#include "DrawComponent.h"
#include "Game.h"

Line::Line() : Actor() {
    mDraw = CreateComponent<DrawComponent>();
}

void Line::HandleRender() {
    bool steep = Math::Abs(mSlope.y) > Math::Abs(mSlope.x);

    float x1, y1, x2, y2;
    if (steep) {
        float ratio = Math::NearlyZero(mSlope.y) ? 0.0f : mSlope.x / mSlope.y;
        y1 = 0.0f;
        x1 = mOrigin.x + ratio * (y1 - mOrigin.y);
        y2 = Game::WINDOW_HEIGHT;
        x2 = mOrigin.x + ratio * (y2 - mOrigin.y);
    } else {
        float ratio = Math::NearlyZero(mSlope.x) ? 0.0f : mSlope.y / mSlope.x;
        x1 = 0.0f;
        y1 = mOrigin.y + ratio * (x1 - mOrigin.x);
        x2 = Game::PLOT_WIDTH;
        y2 = mOrigin.y + ratio * (x2 - mOrigin.x);
    }

    mDraw->AddLine(x1, y1, x2, y2,
                   Game::MAX_COLOR, Game::MAX_COLOR, Game::MAX_COLOR, Game::MAX_COLOR);

    Actor::HandleRender();
}
