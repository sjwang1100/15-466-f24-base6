#include "Mode.hpp"

#include "Connection.hpp"
#include "Game.hpp"
#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode(Client &client);
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	void construct_board();
	void load_chessman();	// Load chessman, but does not add to scene
	void add_chessman(uint8_t index, glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f));	// Add chessman to scene
	void chessman_choose();
	uint8_t moving_mode = 0;	// not moving = 0, choosing -> game = 1;
	void move_board(float elapsed);
	void highlight_square(uint8_t x, uint8_t y);
	void unhighlight_square(uint8_t x, uint8_t y);
	Scene::Drawable* temp_dr = nullptr;
	uint8_t map_to_index(float x, float y);
	glm::u8vec2 map_to_index(uint32_t x, uint32_t y);

	bool drag_mode = false;
	bool mouse_hold = false;
	uint32_t mousex, mousey;

	//----- game state -----

	//input tracking for local player:
	Player::Controls controls;

	Scene scene;
	Scene::Camera* camera = nullptr;

	bool is_black = false;

	//enum chesspcs {
	//	W_Rock,
	//	W_Paper,
	//	W_Scissors,
	//	W_Q,
	//	B_Rock,
	//	B_Paper,
	//	B_Scissors,
	//	B_Q
	//};
	Scene::Transform* chessman[8] = { nullptr };

	//latest game state (from server):
	Game game;

	//last message from server:
	std::string server_message;

	//connection to server:
	Client &client;

};
