#include <iostream>
#include <string>
#include "game/game.h"
#include "resource_manager.h"
#include "config.h"
#include "sprite_renderer.h"
#include "tilemap_renderer.h"
#include "tilemap.h"
#include "camera/tileCamera2D.h"
#include "basic_renderer.h"
#include "game/Player.h"
#include "TextRenderer.h"
#include "Helper.hpp"
#include "menu.hpp"
#include <GLFW/glfw3.h>
#include <thread>
#define printf_v(name, vec, prec) std::printf(name ": [%" prec "f, %" prec "f]\n", (vec).x, (vec).y)

using namespace std::placeholders;
using namespace MenuSystem;

// Initialize static Game member variables.
glm::vec2 Game::TileSize = glm::vec2(32.0f, 32.0f);
std::vector<ITileSpace*> Game::tileSpaceObjects;

SpriteRenderer*	 renderer = nullptr;
BasicRenderer*	 basic_renderer = nullptr;
TextRenderer* text_renderer = nullptr;

// Render state variables.
bool wireframe_render = false;
float fps = 0.0f;
float ref_fps = 0.0f;
float n_fps = 0.0f;
float t_fps = 0.0f;

// Temp
Helper::Stopwatch w1;
Helper::Stopwatch w2;
Helper::Stopwatch w3;
MenuObject* menu = nullptr;
MenuRenderer* menu_renderer = nullptr;
MenuManager* menu_manager = nullptr;
std::string sLastAction = "";
bool bShowMenu = false;

// Callbacks.
void onLayerDraw(const Tmx::Map *map, const Tmx::Layer *layer, int n_layer);
void onCameraScale(glm::vec2 scale);

Game::Game(unsigned int width, unsigned int height) : State(GameState::active), Width(width), Height(height), Keys(), KeysProcessed(), BackgroundColor(0.0f), CurrentLevel(-1)
{}
Game::~Game()
{	
	if (renderer)
		delete renderer;
	if (basic_renderer)
		delete basic_renderer;
	if (text_renderer)
		delete text_renderer;
	if (menu)
		delete menu;
	if (menu_renderer)
		delete menu_renderer;
	if (menu_manager)
		delete menu_manager;
	ResourceManager::Clear();
	for (auto& level : this->Levels)
		GameLevel::Delete(level);
}
void Game::SetTileSize(glm::vec2 new_size)
{
	Game::TileSize = new_size;
	std::for_each(tileSpaceObjects.begin(), tileSpaceObjects.end(), [&](ITileSpace* obj) { 
		obj->onTileSizeChanged(new_size); 
	});	
}
void Game::OnResize()
{
	// If resized, update projection matrix to match new width and height of window.
	glm::mat4 projection = glm::ortho(0.0f, (float)this->Width, (float)this->Height, 0.0f, -1.0f, 1.0f);
	ResourceManager::GetShader("sprite").Use().SetMat4("projection", projection);
	projection = glm::ortho(0.0f, (float)Width, (float)Height, 0.0f, -1.0f, 1.0f);
	TileCamera2D::ScreenCoords = glm::vec2((float)this->Width, (float)this->Height);
	ResourceManager::GetShader("basic_render").Use().SetMat4("projection", projection);
}
void Game::ProcessMouse(float xoffset, float yoffset)
{

}
void Game::ProcessScroll(float yoffset)
{
	TileCamera2D::SetScale(TileCamera2D::GetScale() + 0.1f * yoffset);
	Game::SetTileSize(Game::TileSize);
}
void Game::Init()
{
	Game::SetTileSize(glm::vec2(32.0f, 32.0f));
	this->BackgroundColor = glm::vec3(0.0f);

	// Load shaders
	ResourceManager::LoadShader(SHADERS_DIR "SpriteRender.vert", SHADERS_DIR "SpriteRender.frag", nullptr, "sprite");
	ResourceManager::LoadShader(SHADERS_DIR "BasicRender.vert", SHADERS_DIR "BasicRender.frag", nullptr, "basic_render");
	
	// Load textures
	ResourceManager::LoadTexture(ASSETS_DIR "textures/menu_9patch.png", true, "menu_9patch").SetMagFilter(GL_NEAREST).SetMinFilter(GL_NEAREST).UpdateParameters();

	// Projection used for 2D projection.
	glm::mat4 projection = glm::ortho(0.0f, (float)this->Width, (float)this->Height, 0.0f, -1.0f, 1.0f);

	// Initialize sprite renderer.		
	ResourceManager::GetShader("sprite").Use().SetInt("spriteImage", 0);
	ResourceManager::GetShader("sprite").SetMat4("projection", projection);
	renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));

	// Initialize basic renderer.
	ResourceManager::GetShader("basic_render").Use().SetMat4("projection", projection);
	basic_renderer = new BasicRenderer(ResourceManager::GetShader("basic_render"));
	basic_renderer->SetLineWidth(2.0f);

	// Initialize text renderer.
	text_renderer = new TextRenderer(Width, Height);
	text_renderer->Load(ASSETS_DIR "fonts/arial.ttf", 16, GL_NEAREST);

	// Initialize menu renderer.
	menu_renderer = new MenuRenderer(renderer);

	// Initialize menu.
	menu = new MenuObject();
	MenuObject& mo = *menu;
	mo["main"].SetTable(1, 4);
	mo["main"]["Attack"].SetID(101);

	mo["main"]["Magic"].SetTable(1, 2);

	mo["main"]["Magic"]["White"].SetTable(3, 6);
	auto& menuWhiteMagic = mo["main"]["Magic"]["White"];
	menuWhiteMagic["Cure"].SetID(401);
	menuWhiteMagic["Cura"].SetID(402);
	menuWhiteMagic["Curaga"].SetID(403);
	menuWhiteMagic["Esuna"].SetID(404);

	mo["main"]["Magic"]["Black"].SetTable(3, 4);
	auto& menuBlackMagic = mo["main"]["Magic"]["Black"];
	menuBlackMagic["Fire"].SetID(201);
	menuBlackMagic["Fira"].SetID(202);
	menuBlackMagic["Firaga"].SetID(203);
	menuBlackMagic["Blizzard"].SetID(204);
	menuBlackMagic["Blizzara"].SetID(205).Enable(false);
	menuBlackMagic["Blizzaga"].SetID(206).Enable(false);
	menuBlackMagic["Thunder"].SetID(207);
	menuBlackMagic["Thundara"].SetID(208);
	menuBlackMagic["Thundaga"].SetID(209);
	menuBlackMagic["Quake"].SetID(210);
	menuBlackMagic["Quake2"].SetID(211);
	menuBlackMagic["Quake3"].SetID(212); 
	menuBlackMagic["Bio"].SetID(213);
	menuBlackMagic["Bio1"].SetID(214);
	menuBlackMagic["Bio2"].SetID(215);
	menuBlackMagic["Demi"].SetID(216);
	menuBlackMagic["Demi1"].SetID(217);
	menuBlackMagic["Demi2"].SetID(218);

	mo["main"]["Defend"].SetID(102);

	mo["main"]["Items"].SetTable(2, 4).Enable(false);
	mo["main"]["Items"]["Potion"].SetID(301);
	mo["main"]["Items"]["Ether"].SetID(302);
	mo["main"]["Items"]["Elixir"].SetID(303);

	mo["main"]["Escape"].SetID(103);

	mo.Build(text_renderer);

	// Initialize menu manager.
	menu_manager = new MenuManager();
}

void Game::ProcessInput(float dt)
{
	if (Keys[GLFW_KEY_F1] && !KeysProcessed[GLFW_KEY_F1])
	{
		wireframe_render = !wireframe_render;
		if (wireframe_render)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		KeysProcessed[GLFW_KEY_F1] = true;		
	}
	if (Keys[GLFW_KEY_M] && !KeysProcessed[GLFW_KEY_M])
	{
		bShowMenu = !bShowMenu;
		if (bShowMenu)
			menu_manager->Open(&menu->at("main"));
		else
			menu_manager->Close();
		KeysProcessed[GLFW_KEY_M] = true;
	}
	if (Keys[GLFW_KEY_UP] && !KeysProcessed[GLFW_KEY_UP])
	{
		menu_manager->OnUp();
		KeysProcessed[GLFW_KEY_UP] = true;
	}
	if (Keys[GLFW_KEY_DOWN] && !KeysProcessed[GLFW_KEY_DOWN])
	{
		menu_manager->OnDown();
		KeysProcessed[GLFW_KEY_DOWN] = true;
	}
	if (Keys[GLFW_KEY_LEFT] && !KeysProcessed[GLFW_KEY_LEFT])
	{
		menu_manager->OnLeft();
		KeysProcessed[GLFW_KEY_LEFT] = true;
	}
	if (Keys[GLFW_KEY_RIGHT] && !KeysProcessed[GLFW_KEY_RIGHT])
	{
		menu_manager->OnRight();
		KeysProcessed[GLFW_KEY_RIGHT] = true;
	}
	if (Keys[GLFW_KEY_ENTER] && !KeysProcessed[GLFW_KEY_ENTER])
	{
		MenuObject* command = menu_manager->OnConfirm();
		if (command)
		{
			sLastAction = "Selected: " + command->GetName() + " ID: " + std::to_string(command->GetID());
			menu_manager->Close();
		}

		KeysProcessed[GLFW_KEY_ENTER] = true;
	}
	if (Keys[GLFW_KEY_BACKSPACE] && !KeysProcessed[GLFW_KEY_BACKSPACE])
	{
		menu_manager->OnBack();
		KeysProcessed[GLFW_KEY_BACKSPACE] = true;
	}
}
void Game::Update(float dt)
{
	w1.Restart();
	w1.Stop();
	
	// Step update for FPS.
	if (ref_fps < 2.0f) {
		ref_fps += dt;
		n_fps += 1.0f;
		t_fps += 1.0f / dt;
	}
	else {
		fps = t_fps / n_fps;
		t_fps = 0.0f;
		ref_fps = 0.0f;
		n_fps = 0.0f;
	}
}
void Game::Render()
{
	w2.Restart();

	menu_renderer->Draw(*menu_manager, ResourceManager::GetTexture("menu_9patch"), glm::ivec2(250.0f, 250.f), 3.0f);

	w2.Stop();

	// Render DEBUG text
	char buf[256];

	sprintf(buf, "FPS: %.f\nUpdate step: %f ms\nRender step: %f ms\n%s",
		fps, 
		w1.ElapsedMilliseconds(),
		w2.ElapsedMilliseconds(),
		sLastAction.c_str()
	);
	text_renderer->RenderText(std::string(buf), 10.0f, 10.0f, 1.0f);
}

// Callbacks
/// Called AFTER the layer is drawn.
void Game::OnLayerRendered(const Tmx::Map *map, const Tmx::Layer *layer, int n_layer)
{
}
void onCameraScale(glm::vec2 scale)
{
}
