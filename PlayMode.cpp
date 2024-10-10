#include "PlayMode.hpp"
#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <array>

GLuint assets_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > assets_meshes(LoadTagDefault, []() -> MeshBuffer const* {
	MeshBuffer const* ret = new MeshBuffer(data_path("assets.pnct"));
	assets_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > empty_scene(LoadTagDefault, []() -> Scene const* {
	return new Scene(data_path("empty.scene"), [&](Scene& scene, Scene::Transform* transform, std::string const& mesh_name) {
	});
});

void PlayMode::construct_board() {
	Mesh mesh1 = assets_meshes->lookup("Cube.006");

	auto newTrans1 = new Scene::Transform();
	scene.drawables.emplace_back(newTrans1);
	Scene::Drawable& drawable1 = scene.drawables.back();

	drawable1.pipeline = lit_color_texture_program_pipeline;
	drawable1.pipeline.vao = assets_meshes_for_lit_color_texture_program;
	drawable1.pipeline.type = mesh1.type;
	drawable1.pipeline.start = mesh1.start;
	drawable1.pipeline.count = mesh1.count;

	Mesh mesh = assets_meshes->lookup("Cube.001");	// black
	mesh1 = assets_meshes->lookup("Cube.004");	//white

#define BEGIN -1.545f
	//spacing == 0.02m
	float y = BEGIN;
	float x = BEGIN;
#define INCREMENT 1.03f
	for (uint8_t i = 0; i < 4; i++) {
		for (uint8_t j = 0; j < 4; j++) {
			auto newTrans = new Scene::Transform();
			scene.drawables.emplace_back(newTrans);
			Scene::Drawable& drawable = scene.drawables.back();

			drawable.pipeline = lit_color_texture_program_pipeline;
			drawable.pipeline.vao = assets_meshes_for_lit_color_texture_program;
			if ((i + j) % 2 != 0) {
				//black
				drawable.pipeline.type = mesh.type;
				drawable.pipeline.start = mesh.start;
				drawable.pipeline.count = mesh.count;
			}
			else {
				drawable.pipeline.type = mesh1.type;
				drawable.pipeline.start = mesh1.start;
				drawable.pipeline.count = mesh1.count;
			}
			
			drawable.transform->position = glm::vec3(x, y, 0.05f);
						
			y = y + INCREMENT;
		}
		x = x + INCREMENT;
		y = BEGIN;
	}

}

PlayMode::PlayMode(Client &client_) : client(client_), scene(*empty_scene) {
	construct_board();
	load_chessman();

	moving_mode = 0;
	chessman_choose();

	drag_mode = true;

	std::cout << W_Paper << std::endl;

	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	camera->transform->position = glm::vec3(2.0f, -1.5f, 10.0f);
	camera->transform->rotation *= glm::angleAxis(
		glm::radians(-90.0f),
		glm::vec3(1.0f, 0.0f, 0.0f));

	//highlight_square(3, 2);
}

PlayMode::~PlayMode() {
	//for (auto i : scene.drawables) {
	//	delete i;
	//}
	//for (auto i : chessman) {
	//	delete i;
	//}
}

glm::u8vec2 tempBLK = glm::u8vec2(0, 0);

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.repeat) {
			//ignore repeats
		} else if (evt.key.keysym.sym == SDLK_a) {
			controls.left.downs += 1;
			controls.left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			controls.right.downs += 1;
			controls.right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			controls.up.downs += 1;
			controls.up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			controls.down.downs += 1;
			controls.down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			controls.jump.downs += 1;
			controls.jump.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			controls.left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			controls.right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			controls.up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			controls.down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			controls.jump.pressed = false;
			return true;
		}
	}
	else if (mouse_hold && evt.type == SDL_MOUSEMOTION ) {
		mousex = evt.motion.x;
		mousey = evt.motion.y;
		std::cout << mousex << " "<< mousey << std::endl;
		if (drag_mode) {
			if ((mouse_hold && mousex > 96 && mousex < 225) || dragging != -1) {
				uint8_t temp = 0;
				if (dragging == -1) {

					temp = (uint8_t)(((float)mousey - 90.0f) / 133.25f);
					dragging = temp;
				}
				else temp = dragging;
				
				if (is_black) {
					switch (temp)
					{
					case 3:
						chessman[7]->position.x += evt.motion.yrel * 0.01f;
						chessman[7]->position.y += evt.motion.xrel * 0.01f;
						break;
					case 2:
						chessman[6]->position.x += evt.motion.yrel * 0.01f;
						chessman[6]->position.y += evt.motion.xrel * 0.01f;
						break;
					case 1:
						chessman[5]->position.x += evt.motion.yrel * 0.01f;
						chessman[5]->position.y += evt.motion.xrel * 0.01f;
						break;
					case 0:
						chessman[4]->position.x += evt.motion.yrel * 0.01f;
						chessman[4]->position.y += evt.motion.xrel * 0.01f;
						break;
					default:
						break;
					}
				}
				else {
					switch (temp)
					{
					case 3:
						chessman[3]->position.x += evt.motion.yrel * 0.01f;
						chessman[3]->position.y += evt.motion.xrel * 0.01f;
						break;
					case 2:
						chessman[2]->position.x += evt.motion.yrel * 0.01f;
						chessman[2]->position.y += evt.motion.xrel * 0.01f;
						break;
					case 1:
						chessman[1]->position.x += evt.motion.yrel * 0.01f;
						chessman[1]->position.y += evt.motion.xrel * 0.01f;
						break;
					case 0:
						chessman[0]->position.x += evt.motion.yrel * 0.01f;
						chessman[0]->position.y += evt.motion.xrel * 0.01f;
						break;
					default:
						break;
					}
				}
			}
			glm::u8vec2 blk = map_to_index(mousex, mousey);
			if (blk.x < 4 && blk.x >= 0 && blk.y < 4 && blk.y >= 0) {
				if (blk != tempBLK) {
					unhighlight_square(tempBLK.x, tempBLK.y);
					tempBLK = blk;
					highlight_square(blk.x, blk.y);
				}
				
			}
			else {
				unhighlight_square(blk.x, blk.y);
			}
		}
	}
	else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		mouse_hold = true;
		return true;
	}
	else if (evt.type == SDL_MOUSEBUTTONUP) {
		mouse_hold = false;
		if (dragging != -1) {

		}
		dragging = -1;
		map_to_index(mousex, mousey);
		return true;
	}

	
	return false;
}

void PlayMode::update(float elapsed) {
	move_board(elapsed);
	//queue data for sending to server:
	controls.send_controls_message(&client.connection);

	

	//reset button press counters:
	controls.left.downs = 0;
	controls.right.downs = 0;
	controls.up.downs = 0;
	controls.down.downs = 0;
	controls.jump.downs = 0;

	//send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			//std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush(); //DEBUG
			bool handled_message;
			try {
				do {
					handled_message = false;
					if (game.recv_state_message(c)) handled_message = true;
				} while (handled_message);
			} catch (std::exception const &e) {
				std::cerr << "[" << c->socket << "] malformed message from server: " << e.what() << std::endl;
				//quit the game:
				throw e;
			}
		}
	}, 0.0);
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	/* In case you are wondering if your walkmesh is lining up with your scene, try:
	{
		glDisable(GL_DEPTH_TEST);
		DrawLines lines(player.camera->make_projection() * glm::mat4(player.camera->transform->make_world_to_local()));
		for (auto const &tri : walkmesh->triangles) {
			lines.draw(walkmesh->vertices[tri.x], walkmesh->vertices[tri.y], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
			lines.draw(walkmesh->vertices[tri.y], walkmesh->vertices[tri.z], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
			lines.draw(walkmesh->vertices[tri.z], walkmesh->vertices[tri.x], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
		}
	}
	*/

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		/*lines.draw_text("Mouse motion looks; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));*/
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("For now, use mouse to drag pieces to the board",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		//lines.draw_text("Height: " + std::to_string(1),
		//	glm::vec3(-aspect + 0.1f * H + ofs, /*-1.0 + + 0.1f * H + ofs*/0.9, 0.0),
		//	glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
		//	glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
	//static std::array< glm::vec2, 16 > const circle = [](){
	//	std::array< glm::vec2, 16 > ret;
	//	for (uint32_t a = 0; a < ret.size(); ++a) {
	//		float ang = a / float(ret.size()) * 2.0f * float(M_PI);
	//		ret[a] = glm::vec2(std::cos(ang), std::sin(ang));
	//	}
	//	return ret;
	//}();

	//glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);
	//glDisable(GL_DEPTH_TEST);
	//
	////figure out view transform to center the arena:
	//float aspect = float(drawable_size.x) / float(drawable_size.y);
	//float scale = std::min(
	//	2.0f * aspect / (Game::ArenaMax.x - Game::ArenaMin.x + 2.0f * Game::PlayerRadius),
	//	2.0f / (Game::ArenaMax.y - Game::ArenaMin.y + 2.0f * Game::PlayerRadius)
	//);
	//glm::vec2 offset = -0.5f * (Game::ArenaMax + Game::ArenaMin);

	//glm::mat4 world_to_clip = glm::mat4(
	//	scale / aspect, 0.0f, 0.0f, offset.x,
	//	0.0f, scale, 0.0f, offset.y,
	//	0.0f, 0.0f, 1.0f, 0.0f,
	//	0.0f, 0.0f, 0.0f, 1.0f
	//);

	//scene.draw(*player.camera);

	//{
	//	DrawLines lines(world_to_clip);

	//	//helper:
	//	auto draw_text = [&](glm::vec2 const &at, std::string const &text, float H) {
	//		lines.draw_text(text,
	//			glm::vec3(at.x, at.y, 0.0),
	//			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
	//			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	//		float ofs = (1.0f / scale) / drawable_size.y;
	//		lines.draw_text(text,
	//			glm::vec3(at.x + ofs, at.y + ofs, 0.0),
	//			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
	//			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	//	};

	//	lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMin.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
	//	lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMax.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
	//	lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMin.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
	//	lines.draw(glm::vec3(Game::ArenaMax.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));

	//	for (auto const &player : game.players) {
	//		glm::u8vec4 col = glm::u8vec4(player.color.x*255, player.color.y*255, player.color.z*255, 0xff);
	//		//if (&player == &game.players.front()) {
	//		//	//mark current player (which server sends first):
	//		//	lines.draw(
	//		//		glm::vec3(player.position + Game::PlayerRadius * glm::vec2(-0.5f,-0.5f), 0.0f),
	//		//		glm::vec3(player.position + Game::PlayerRadius * glm::vec2( 0.5f, 0.5f), 0.0f),
	//		//		col
	//		//	);
	//		//	lines.draw(
	//		//		glm::vec3(player.position + Game::PlayerRadius * glm::vec2(-0.5f, 0.5f), 0.0f),
	//		//		glm::vec3(player.position + Game::PlayerRadius * glm::vec2( 0.5f,-0.5f), 0.0f),
	//		//		col
	//		//	);
	//		//}
	//		//for (uint32_t a = 0; a < circle.size(); ++a) {
	//		//	lines.draw(
	//		//		glm::vec3(player.position + Game::PlayerRadius * circle[a], 0.0f),
	//		//		glm::vec3(player.position + Game::PlayerRadius * circle[(a+1)%circle.size()], 0.0f),
	//		//		col
	//		//	);
	//		//}

	//		draw_text(player.position + glm::vec2(0.0f, -0.1f + Game::PlayerRadius), player.name, 0.09f);
	//	}
	//}
	//GL_ERRORS();
}


std::string lookups[] = { "Cylinder.003", "Cylinder.009", "Cylinder.001", "Text",
							"Cylinder.004", "Cylinder.005", "Cylinder.002", "Text.001" };
// Load chessman, but does not add to scene
void PlayMode::load_chessman() {

	uint8_t j = 0;
	for (auto i : lookups) {
		//Mesh mesh = assets_meshes->lookup(i);

		auto newTrans = new Scene::Transform();
		chessman[j] = newTrans;

		j++;
	}
	
}

void PlayMode::add_chessman(uint8_t index, glm::vec3 pos) {
	Mesh mesh = assets_meshes->lookup(lookups[index]);
	scene.drawables.emplace_back(chessman[index]);
	Scene::Drawable& drawable = scene.drawables.back();

	drawable.pipeline = lit_color_texture_program_pipeline;
	drawable.pipeline.vao = assets_meshes_for_lit_color_texture_program;
	drawable.pipeline.type = mesh.type;
	drawable.pipeline.start = mesh.start;
	drawable.pipeline.count = mesh.count;
	//drawable.transform->parent = player.transform;
	drawable.transform->position = pos;
	
	// Not sure why these 4 are weirdly oriented
	if (index == 5 || index == 1) {
		chessman[index]->rotation *= glm::angleAxis(
			glm::radians(-90.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
		chessman[index]->rotation *= glm::angleAxis(
			glm::radians(-20.0f),
			glm::vec3(1.0f, 0.0f, 0.0f));
	}
	else if (index == 3 || index == 7) {
		chessman[index]->rotation *= glm::angleAxis(
			glm::radians(90.0f),
			glm::vec3(0.0f, 0.0f, 1.0f));
	}
}

void PlayMode::move_board(float elapsed) {
	const float speed_factory = 2.0f;
	const float speed_factorx = 2.5f;

	//camera->transform->position = glm::vec3(0.0f, 0.0f, 10.0f);

	if (moving_mode == 1) {
		if (camera->transform->position.x <= 0.0f && camera->transform->position.y >= 0.0f) {
			moving_mode = 0;
			return;
		}
		camera->transform->position.x -= speed_factorx * elapsed;
		camera->transform->position.y += speed_factory * elapsed;
	}
}

void PlayMode::chessman_choose() {
	if (is_black) {
		add_chessman(4, glm::vec3(0.5f, -4.5f, 0.2f));
		add_chessman(5, glm::vec3(1.2f, -4.4f, 0.2f));
		add_chessman(6, glm::vec3(2.6f, -4.5f, 0.2f));
		add_chessman(7, glm::vec3(3.7f, -4.7f, 0.2f));
	}
	else {
		add_chessman(0, glm::vec3(0.5f, -4.5f, 0.2f));
		add_chessman(1, glm::vec3(1.2f, -4.4f, 0.2f));
		add_chessman(2, glm::vec3(2.6f, -4.5f, 0.2f));
		add_chessman(3, glm::vec3(3.7f, -4.7f, 0.2f));
	}
	
}


void PlayMode::highlight_square(uint8_t x, uint8_t y) {
	x++;
	uint8_t index = 4 * (3-y)  + x;

	//from ChatGPT
	std::list<Scene::Drawable>::iterator it = scene.drawables.begin();
	std::advance(it, index);
	/*it++;
	it++;*/
	if (temp_dr != nullptr) {
		return;		//should be something else
	}

	glm::vec3 tempPOS = it->transform->position;
	// not from ChatGPT
	//std::swap(*it, temp_dr);

	Mesh mesh = assets_meshes->lookup("Cube.005");	// black
	auto newTrans = new Scene::Transform();
	temp_dr = new Scene::Drawable(newTrans);

	temp_dr->pipeline = lit_color_texture_program_pipeline;
	temp_dr->pipeline.vao = assets_meshes_for_lit_color_texture_program;
	temp_dr->pipeline.type = mesh.type;
	temp_dr->pipeline.start = mesh.start;
	temp_dr->pipeline.count = mesh.count;
	temp_dr->transform->position = tempPOS;

	std::swap(*it, *temp_dr);
}

void PlayMode::unhighlight_square(uint8_t x, uint8_t y) {
	if (temp_dr == nullptr) {
		return;
	}

	x++;
	uint8_t index = 4 * (3 - y) + x;

	//from ChatGPT
	std::list<Scene::Drawable>::iterator it = scene.drawables.begin();
	std::advance(it, index);

	// not from ChatGPT
	//delete it;

	std::swap(*it, *temp_dr);
	delete temp_dr;
	temp_dr = nullptr;
}

uint8_t PlayMode::map_to_index(float x, float y) {
	return 1 ;
}
glm::u8vec2 PlayMode::map_to_index(uint32_t x, uint32_t y) {
	x -= 310;
	float x1 = (float)x / 156.5f;

	y -= 50;
	float y1 = (float)y / 155.5f;

	y = 3 - (uint8_t)y1;

	//std::cout << (int)x1 << " " << y << std::endl;
	return glm::u8vec2((int)x1, y);
}