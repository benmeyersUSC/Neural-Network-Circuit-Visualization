#include "Game.h"

#include <iostream>

#include "Actor.h"
#include "Component.h"
#include "DrawComponent.h"
#include "Line.h"
#include "NeuralNetworkActor.h"
Game gGame;
#include "NeuralNetwork.h"
Game::Game()
{
	// comments in class declaration explain these members

	mSdlWindow = nullptr;
	mSdlRenderer = nullptr;

	mContinueRunning = true;

	mPreviousTime = 0;
}

bool Game::Initialize()
{
	SDL_SetHint("SDL_MAIN_CALLBACK_RATE", "60");
	bool sdlInit = SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
	// fail if SDL init doesn't work
	if (!sdlInit)
	{
		return false;
	}
	// use window constants
	mSdlWindow = SDL_CreateWindow("NEURAL NETWORK CIRCUITS", PLOT_WIDTH, WINDOW_HEIGHT, 0);
	if (mSdlWindow == nullptr)
	{
		return false;
	}

	mSdlRenderer = SDL_CreateRenderer(mSdlWindow, nullptr);
	if (mSdlRenderer == nullptr)
	{
		return false;
	}

	// init actors!
	LoadData();
	return true;
}

bool Game::RunIteration()
{
	// game loop!

	// get input
	ProcessInput();
	// update game objects
	UpdateGame();
	// render
	GenerateOutput();
	return mContinueRunning;
}

void Game::Shutdown()
{
	// call proper unloaders/destroyers
	UnloadData();
	SDL_DestroyRenderer(mSdlRenderer);
	SDL_DestroyWindow(mSdlWindow);
	SDL_Quit();
}

void Game::HandleEvent(const SDL_Event* event)
{
	// x button
	if (event->type == SDL_EVENT_QUIT)
	{
		mContinueRunning = false;
	}
}

void Game::DestroyActor(Actor *actor) {

		std::erase(mActors, actor);
		delete actor;

}

void Game::ProcessInput()
{
	const bool* keyboardState = SDL_GetKeyboardState(nullptr);
	// esc key can also end game
	if (keyboardState[SDL_SCANCODE_ESCAPE])
	{
		mContinueRunning = false;
	}

	// mouse
	SDL_MouseButtonFlags mouseButtons = SDL_GetMouseState(&mMousePos.x, &mMousePos.y);

	for (auto a : mActors) {
		a->Input(keyboardState, mouseButtons, mMousePos);
	}

}

void Game::AddRenderable(Component* comp) {
	const auto it = std::ranges::upper_bound(mRenderables, comp,
	                                   [](const Component* a, const Component* b) {
		                                   return a->GetDrawOrder() < b->GetDrawOrder();
	                                   });
	mRenderables.insert(it, comp);
}

void Game::RemoveRenderable(Component* comp) {
	std::erase(mRenderables, comp);
}

void Game::LeadingEdge(bool keyBool, bool &lastBool, const std::function<void()> &fn, bool condition) {
	if (!lastBool && keyBool && condition) {
		fn();
	}
	lastBool = keyBool;
}

void Game::AddPendingDestroy(class Actor *actor) {
	if (std::ranges::find(mPendingDestroy, actor) == mPendingDestroy.end())
	{
		mPendingDestroy.emplace_back(actor);
	}
}


void Game::UpdateGame()
{
	// calculate deltatime
	Uint64 currTimeMs = SDL_GetTicks();
	Uint64 uIntDiff = currTimeMs - mPreviousTime;
	mPreviousTime = currTimeMs;
	float deltaTime = static_cast<float>(uIntDiff) / MS_PER_SEC;
	deltaTime = Math::Min(MAX_DELTA_TIME, deltaTime);
	mDT += deltaTime;

	// 'leading edge' of elapsed eclipsing set duration
	if (mDT >= DURATION_SECONDS && !mGameDone) {
		mGameDone = true;
	}

	for (auto actor : mPendingCreate)
		mActors.emplace_back(actor);

	mPendingCreate.clear();

	for (auto actor : mActors)
		actor->Update(deltaTime);

	for (auto actor : mPendingDestroy)
		DestroyActor(actor);

	mPendingDestroy.clear();
}

void Game::GenerateOutput()
{
	SDL_SetRenderDrawColor(mSdlRenderer, 0, 0, 0, MAX_COLOR);
	SDL_RenderClear(mSdlRenderer);

	// actors call their own HandleRender, creating shapes
	for (auto& a : mActors) {
		a->Render();
	}

	// actual rendering components then draw shapes to SDL in order
	for (auto& comp : mRenderables) {
		comp->Render();
	}

	SDL_RenderPresent(mSdlRenderer);
}

void Game::LoadData()
{
	mNN = CreateActor<NeuralNetworkActor>();
	mNN->GetNN().FromConfig(NN_CFG_PATH);
	mNN->SetWidth(927.0f);
	mNN->SetHeight(549.0f);
	mNN->GetTransform().SetPosition({HALF_WIDTH, HALF_HEIGHT});
	mNN->StartGraphicForward();
}

void Game::UnloadData()
{
	// delete objects and clear vector
	for (auto& actor : mActors)
	{
		delete actor;
	}
	mActors.clear();
}

