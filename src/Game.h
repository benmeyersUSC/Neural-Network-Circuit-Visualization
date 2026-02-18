//
// Created by Ben Meyers on 2/11/26.
//

#ifndef RELATIVITY_GAME_H
#define RELATIVITY_GAME_H

#pragma once

#include "SDL3/SDL.h"
#include "Math.h"
#include <functional>
#include <vector>
#include "Actor.h"

class Component;
class DrawComponent;
using Math::Vector2;

class Game
{
public:

	static constexpr float DURATION_SECONDS = 10.0f;

	static constexpr unsigned ESTIMATED_FPS = 60;


	// window constants
	static constexpr float PLOT_WIDTH = 1107.0f;
	static constexpr float SIDEBAR_WIDTH = 363.0f;
	static constexpr float WINDOW_WIDTH = PLOT_WIDTH + SIDEBAR_WIDTH;
	static constexpr float WINDOW_HEIGHT = 855.0f;
	static constexpr float HALF_WIDTH = PLOT_WIDTH/2.0f;
	static constexpr float HALF_HEIGHT = WINDOW_HEIGHT/2.0f;

	// max deltaTime for update
	static constexpr float MAX_DELTA_TIME = 0.033f;

	// constant for full color
	static constexpr Uint8 MAX_COLOR = 255;

	// constant for half size of character pixel count (according to SDL)
	static constexpr float CHAR_PIXELS = 8.0f;
	static constexpr float HALF_CHAR_PIXELS = 4.0f;

	// constant for 1000 ms in one s
	static constexpr float MS_PER_SEC = 1000.0f;



	Game();

	// Initialize the game
	// Returns true if successful
	bool Initialize();

	// Runs an interation of the game loop
	// Returns true if the game loop should continue
	bool RunIteration();

	// Called when the game gets shutdown
	void Shutdown();

	// Called when the game receives an event from SDL
	void HandleEvent(const SDL_Event* event);

	template<typename A>
	A* CreateActor() {
		// create actor on heap, save pointer to vector
		A* a = new A();
		mActors.push_back(a);
		return a;
	}
	void AddPendingDestroy(class Actor* actor);

	void AddRenderable(Component* comp);
	void RemoveRenderable(Component* comp);

	[[nodiscard]] const Vector2& GetMousePos()const{return mMousePos;}

	SDL_Renderer* GetRenderer(){return mSdlRenderer;}
	[[nodiscard]] float GetDT()const{return mDT;}
private:
	// window and renderer
	SDL_Window* mSdlWindow;
	SDL_Renderer* mSdlRenderer;

	// keep the game going
	bool mContinueRunning;
	bool mGameDone = false;


	// actors vector and individual member variables
	std::vector<Actor*> mActors;
	std::vector<Actor*> mPendingCreate;
	std::vector<Actor*> mPendingDestroy;
	std::vector<Component*> mRenderables;

	void DestroyActor(Actor* actor);


	// prev time for delta calcs
	Uint64 mPreviousTime;

	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();

	void LoadData();
	void UnloadData();



	float mDT = 0.0f;
	Vector2 mMousePos;

};

extern Game gGame;

#endif //RELATIVITY_GAME_H