#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>

GLuint game_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > game_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("game.pnct"));
	game_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > game_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("game.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = game_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = game_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

WalkMesh const *walkmesh = nullptr;
Load< WalkMeshes > game_walkmeshes(LoadTagDefault, []() -> WalkMeshes const * {
	WalkMeshes *ret = new WalkMeshes(data_path("game.w"));
	walkmesh = &ret->lookup("Walkmesh");
	return ret;
});

void PlayMode::spawn_virus() {
	int i = -1;
	for (int j = spawn_num; j < spawn_num + num_viruses; j++) {
		if (viruses[j % num_viruses].state == DEAD) {
			i = j % num_viruses;
			spawn_num++;
			if (spawn_num >= num_viruses) spawn_num -= num_viruses;
			break;
		}
	}
	if (i == -1) return;

	viruses[i].state = ALIVE;
	float theta = 2.0f * float(M_PI) * (mt() / float(mt.max()));
	float phi = acos(1.0f - 2.0f * (mt() / float(mt.max())));
	viruses[i].transform->position = spawn_distance * glm::vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
	viruses[i].direction = -glm::normalize(viruses[i].transform->position);

	viruses[i].dlong = int(glm::degrees(theta)) - 180;
	viruses[i].dlat = -int(glm::degrees(phi)) + 90;

	theta = 2.0f * float(M_PI) * (mt() / float(mt.max()));
	phi = acos(1.0f - 2.0f * (mt() / float(mt.max())));
	viruses[i].rotate_axis = glm::vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
	viruses[i].speed = virus_speed;

}

void PlayMode::kill_virus(int i) {
	viruses[i].state = DEAD;
	viruses[i].transform->position = glm::vec3(0.0f, 0.0f, 0.0f);
}

void PlayMode::update_viruses(float elapsed) {
	for (int i = 0; i < num_viruses; i++) {
		if (viruses[i].state == ALIVE) {
			viruses[i].transform->position += viruses->speed * elapsed * viruses[i].direction;
			if (glm::length(viruses[i].transform->position) < 3.0f) {
				kill_virus(i);
				lives--;
				if (lives == 0) {
					reset_game = true;
					dead = true;
					break;
				}
			}
			viruses[i].transform->rotation = glm::angleAxis(viruses[i].speed * elapsed, viruses[i].rotate_axis) * viruses[i].transform->rotation;
		}
	}
}

bool PlayMode::tip_near_virus() {
	glm::mat4x3 frame = tip->make_local_to_world();
	glm::vec3 forward = -glm::normalize(frame[0]);
	glm::vec3 right = glm::normalize(frame[1]);
	glm::vec3 up = glm::normalize(frame[2]);
	glm::vec3 pos = forward * tip->position.x + right * tip->position.y + up * tip->position.z;
	pos = pos * 2.5f;
	pos = pos + player.transform->position;
	for (int i = 0; i < num_viruses; i++) {
		if (viruses[i].state == ALIVE && glm::distance(pos, viruses[i].transform->position) < neutralize_dist)
			return true;
	}
	return false;
}

void PlayMode::neutralize() {
	glm::mat4x3 frame = tip->make_local_to_world();
	glm::vec3 forward = -glm::normalize(frame[0]);
	glm::vec3 right = glm::normalize(frame[1]);
	glm::vec3 up = glm::normalize(frame[2]);
	glm::vec3 pos = forward * tip->position.x + right * tip->position.y + up * tip->position.z;
	pos = pos * 2.5f;
	pos = pos + player.transform->position;
	for (int i = 0; i < num_viruses; i++) {
		if (viruses[i].state == ALIVE && glm::distance(pos, viruses[i].transform->position) < neutralize_dist)
			kill_virus(i);
	}
}

void PlayMode::resetGame() {
	reset_game = false;

	virus_speed = 1.5f;
	virus_spawn_timer = 0.0f;
	virus_speed_timer = 0.0f;

	player_speed = 30.0f;

	if (int(game_timer) > high_score)
		high_score = int(game_timer);
	game_timer = 0.0f;
	lives = 5;

	player.at = walkmesh->nearest_walk_point(glm::vec3(0.0f, 0.0f, 3.0f));

	for (int i = 0; i < num_viruses; i++) {
		kill_virus(i);
	}

	spawn_virus();
}

PlayMode::PlayMode() : scene(*game_scene) {
	//create a player transform:
	//scene.transforms.emplace_back();
	//player.transform = &scene.transforms.back();
	for (auto& transform : scene.transforms) {
		if (transform.name == "Player") player.transform = &transform;
		else if (transform.name == "Syringe") syringe = &transform;
		else if (transform.name == "Tip") tip = &transform;
		else if (transform.name == "Virus0") viruses[0].transform = &transform;
		else if (transform.name == "Virus1") viruses[1].transform = &transform;
		else if (transform.name == "Virus2") viruses[2].transform = &transform;
		else if (transform.name == "Virus3") viruses[3].transform = &transform;
		else if (transform.name == "Virus4") viruses[4].transform = &transform;
	}
	if (player.transform == nullptr) throw std::runtime_error("GameObject not found.");
	if (syringe == nullptr) throw std::runtime_error("GameObject not found.");
	if (tip == nullptr) throw std::runtime_error("GameObject not found.");
	for (int i = 0; i < num_viruses; i++) {
		if (viruses[i].transform == nullptr) throw std::runtime_error("GameObject not found.");
	}

	//create a player camera attached to a child of the player transform:
	scene.transforms.emplace_back();
	scene.cameras.emplace_back(&scene.transforms.back());
	player.camera = &scene.cameras.back();
	player.camera->fovy = glm::radians(60.0f);
	player.camera->near = 0.01f;
	player.camera->transform->parent = player.transform;
	//player.camera->transform->parent = syringe;

	//player's eyes are 1.8 units above the ground:
	player.camera->transform->position = glm::vec3(0.0f, -15.0f, 40.0f);

	//rotate camera facing direction (-z) to player facing direction (+y):
	player.camera->transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	//start player walking at nearest walk point:
	player.at = walkmesh->nearest_walk_point(glm::vec3(0.0f, 0.0f, 3.0f));

	for (int i = 0; i < num_viruses; i++) {
		kill_virus(i);
	}

	spawn_virus();

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_LSHIFT) {
			shift.downs += 1;
			shift.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_LSHIFT) {
			shift.pressed = false;
			return true;
		}
	}
	else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (evt.button.button == SDL_BUTTON_RIGHT && SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			looking = true;
			return true;
		}
		else if (evt.button.button == SDL_BUTTON_LEFT && !mouse.pressed) {
			mouse.downs += 1;
			mouse.pressed= true;
		}
	}
	else if (evt.type == SDL_MOUSEBUTTONUP) {
		if (evt.button.button == SDL_BUTTON_RIGHT && SDL_GetRelativeMouseMode() == SDL_TRUE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			looking = false;
			return true;
		}
		else if (evt.button.button == SDL_BUTTON_LEFT) {
			mouse.pressed = false;
		}
	}else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			glm::vec3 up = walkmesh->to_world_smooth_normal(player.at);
			player.transform->rotation = glm::angleAxis(-motion.x * player.camera->fovy, up) * player.transform->rotation;

			float pitch = glm::pitch(player.camera->transform->rotation);
			pitch += motion.y * player.camera->fovy;
			//camera looks down -z (basically at the player's feet) when pitch is at zero.
			pitch = std::min(pitch, 0.95f * 3.1415926f);
			pitch = std::max(pitch, 0.05f * 3.1415926f);
			player.camera->transform->rotation = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));

			return true;
		}
	}
	

	return false;
}

void PlayMode::update(float elapsed) {
	//player walking:
	{
		//combine inputs into a move:
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * player_speed * elapsed;

		//get move in world coordinate system:
		glm::vec3 remain = player.transform->make_local_to_world() * glm::vec4(move.x, move.y, 0.0f, 0.0f);

		//using a for() instead of a while() here so that if walkpoint gets stuck in
		// some awkward case, code will not infinite loop:
		for (uint32_t iter = 0; iter < 10; ++iter) {
			if (remain == glm::vec3(0.0f)) break;
			WalkPoint end;
			float time;
			walkmesh->walk_in_triangle(player.at, remain, &end, &time);
			player.at = end;
			if (time == 1.0f) {
				//finished within triangle:
				remain = glm::vec3(0.0f);
				break;
			}
			//some step remains:
			remain *= (1.0f - time);
			//try to step over edge:
			glm::quat rotation;
			if (walkmesh->cross_edge(player.at, &end, &rotation)) {
				//stepped to a new triangle:
				player.at = end;
				//rotate step to follow surface:
				remain = rotation * remain;
			} else {
				//ran into a wall, bounce / slide along it:
				glm::vec3 const &a = walkmesh->vertices[player.at.indices.x];
				glm::vec3 const &b = walkmesh->vertices[player.at.indices.y];
				glm::vec3 const &c = walkmesh->vertices[player.at.indices.z];
				glm::vec3 along = glm::normalize(b-a);
				glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));
				glm::vec3 in = glm::cross(normal, along);

				//check how much 'remain' is pointing out of the triangle:
				float d = glm::dot(remain, in);
				if (d < 0.0f) {
					//bounce off of the wall:
					remain += (-1.25f * d) * in;
				} else {
					//if it's just pointing along the edge, bend slightly away from wall:
					remain += 0.01f * d * in;
				}
			}
		}

		if (remain != glm::vec3(0.0f)) {
			std::cout << "NOTE: code used full iteration budget for walking." << std::endl;
		}

		//update player's position to respect walking:
		player.transform->position = walkmesh->to_world_point(player.at);

		{ //update player's rotation to respect local (smooth) up-vector:
			
			glm::quat adjust = glm::rotation(
				player.transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f), //current up vector
				walkmesh->to_world_smooth_normal(player.at) //smoothed up vector at walk location
			);
			player.transform->rotation = glm::normalize(adjust * player.transform->rotation);
		}

		float dist = glm::length(player.transform->position);

		player_long = -int(glm::degrees(atan2(player.transform->position.y, player.transform->position.x)));
		player_lat = -int(glm::degrees(acos(player.transform->position.z / dist))) + 90;

		if (space.pressed && !shift.pressed) syringe_pitch += syringe_tilt_rate * elapsed;
		else if (shift.pressed && !space.pressed) syringe_pitch -= syringe_tilt_rate * elapsed;
		if (syringe_pitch > syringe_max_tilt) syringe_pitch = syringe_max_tilt;
		else if (syringe_pitch < syringe_min_tilt) syringe_pitch = syringe_min_tilt;
		//update syringe
		syringe->rotation = glm::angleAxis(glm::radians(syringe_pitch), glm::vec3(1.0f, 0.0f, 0.0f));

		/*
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 forward = -frame[2];

		camera->transform->position += move.x * right + move.y * forward;
		*/
	}

	virus_spawn_timer += elapsed;
	if (virus_spawn_timer > virus_spawn_rate) {
		spawn_virus();
		virus_spawn_timer = 0.0f;
	}

	virus_speed_timer += elapsed;
	if (virus_speed_timer > virus_speed_rate) {
		if (virus_speed < virus_max_speed) virus_speed += virus_speed_increase;
		if (player_speed < player_max_speed) player_speed += player_speed_increase;
		if (virus_spawn_rate > min_spawn_rate) virus_spawn_rate -= virus_spawn_rate_decrease;
		virus_speed_timer = 0.0f;
	}

	if (mouse.downs > 0) {
		neutralize();
	}

	if (dead) {
		dead_timer += elapsed;
		if (dead_timer > dead_rate) {
			dead_timer = 0.0f;
			dead = false;
		}
	}

	game_timer += elapsed;

	update_viruses(elapsed);

	if (reset_game) resetGame();
	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	space.downs = 0;
	shift.downs = 0;
	mouse.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	player.camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*player.camera);

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
		lines.draw_text("RMB looks; WASD moves; LShift and space tilt; LMB to neutralize viruses",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("RMB looks; WASD moves; LShift and space tilt; LMB to neutralize viruses",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		if (tip_near_virus()) {
			lines.draw_text("Click to neutralize!",
				glm::vec3(-0.2f - 0.1 * H, -0.6f + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw_text("Click to neutralize!",
				glm::vec3(-0.2f - 0.1 * H + ofs, -0.6f + 0.1f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0x22, 0x22, 0x00));
		}


		if (player_lat < 0) {
			lines.draw(
				glm::vec3(-0.20f - 0.1 * H, -0.65f - 0.1f * H, 0.0),
				glm::vec3(-0.15f - 0.1 * H, -0.65f - 0.1f * H, 0.0),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw(
				glm::vec3(-0.20f - 0.1 * H + ofs, -0.65f - 0.1f * H + ofs, 0.0),
				glm::vec3(-0.15f - 0.1 * H + ofs, -0.65f - 0.1f * H + ofs, 0.0),
				glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		}
		lines.draw_text(std::to_string(abs(player_lat)) + " N ",
			glm::vec3(-0.15f - 0.1 * H, -0.7f - 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text(std::to_string(abs(player_lat)) + " N ",
			glm::vec3(-0.15f - 0.1 * H + ofs, -0.7f - 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));


		if (player_long < 0) {
			lines.draw(
				glm::vec3(0.1f - 0.1 * H, -0.65f - 0.1f * H, 0.0),
				glm::vec3(0.15f - 0.1 * H, -0.65f - 0.1f * H, 0.0),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw(
				glm::vec3(0.1f - 0.1 * H + ofs, -0.65f - 0.1f * H + ofs, 0.0),
				glm::vec3(0.15f - 0.1 * H + ofs, -0.65f - 0.1f * H + ofs, 0.0),
				glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		}
		lines.draw_text(std::to_string(abs(player_long)) + " W",
			glm::vec3(0.15f - 0.1 * H, -0.7f - 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text(std::to_string(abs(player_long)) + " W",
			glm::vec3(0.15f - 0.1 * H + ofs, -0.7f - 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		for (int i = 0; i < num_viruses; i++) {

			if (viruses[i].state == ALIVE) {

				int draw_lat = viruses[i].dlat;
				int draw_long = viruses[i].dlong;

				lines.draw_text(std::string(colors[i]) + " virus at",
					glm::vec3(-1.0f * aspect + 0.1f * H, 0.75 - 1.1f * H * (i + 1), 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0xff, 0x22, 0x22, 0x00));
				lines.draw_text(std::string(colors[i]) + " virus at",
					glm::vec3(-1.0f * aspect + 0.1f * H + ofs, 0.75 - 1.1f * H * (i + 1) + ofs, 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0xff, 0xff, 0xff, 0x00));
				if (draw_lat < 0) {
					lines.draw(
						glm::vec3(-0.7f * aspect + 0.1f * H, 0.8 - 1.1f * H * (i + 1), 0.0),
						glm::vec3(-0.66f * aspect + 0.1f * H, 0.8 - 1.1f * H * (i + 1), 0.0),
						glm::u8vec4(0xff, 0x22, 0x22, 0x00));
					lines.draw(
						glm::vec3(-0.7f * aspect + 0.1f * H + ofs, 0.8 - 1.1f * H * (i + 1) + ofs, 0.0),
						glm::vec3(-0.66f * aspect + 0.1f * H + ofs, 0.8 - 1.1f * H * (i + 1) + ofs, 0.0),
						glm::u8vec4(0xff, 0xff, 0xff, 0x00));
				}
				lines.draw_text(std::to_string(abs(draw_lat)) + " N ",
					glm::vec3(-0.65f * aspect - 0.1 * H, 0.75 - 1.1f * H * (i + 1), 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0xff, 0x22, 0x22, 0x00));
				lines.draw_text(std::to_string(abs(draw_lat)) + " N ",
					glm::vec3(-0.65f * aspect - 0.1 * H + ofs, 0.75 - 1.1f * H * (i + 1) + ofs, 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0xff, 0xff, 0xff, 0x00));
				if (draw_long < 0) {
					lines.draw(
						glm::vec3(-0.5f * aspect + 0.1f * H, 0.8 - 1.1f * H * (i + 1), 0.0),
						glm::vec3(-0.46f * aspect + 0.1f * H, 0.8 - 1.1f * H * (i + 1), 0.0),
						glm::u8vec4(0xff, 0x22, 0x22, 0x00));
					lines.draw(
						glm::vec3(-0.5f * aspect + 0.1f * H + ofs, 0.8 - 1.1f * H * (i + 1) + ofs, 0.0),
						glm::vec3(-0.46f * aspect + 0.1f * H + ofs, 0.8 - 1.1f * H * (i + 1) + ofs, 0.0),
						glm::u8vec4(0xff, 0xff, 0xff, 0x00));
				}
				lines.draw_text(std::to_string(abs(draw_long)) + " W",
					glm::vec3(-0.45f * aspect - 0.1 * H, 0.75 - 1.1f * H * (i + 1), 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0xff, 0x22, 0x22, 0x00));
				lines.draw_text(std::to_string(abs(draw_long)) + " W",
					glm::vec3(-0.45f * aspect - 0.1 * H + ofs, 0.75 - 1.1f * H * (i + 1) + ofs, 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			}
		}

		lines.draw_text("Time survived: " + std::to_string(int(game_timer)),
			glm::vec3(0.65 * aspect, 1.0 - 1.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text("Time survived: " + std::to_string(int(game_timer)),
			glm::vec3(0.65 * aspect + ofs, 1.0 - 1.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		lines.draw_text("Lives: " + std::to_string(lives),
			glm::vec3(-aspect + 1.0f * H, 1.0 - 1.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text("Lives: " + std::to_string(lives),
			glm::vec3(-aspect + 1.0f * H + ofs, 1.0 - 1.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0x33, 0x33, 0x00));

		lines.draw_text("Highscore: " + std::to_string(high_score),
			glm::vec3(0.65f * aspect, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text("Highscore: " + std::to_string(high_score),
			glm::vec3(0.65f * aspect + ofs, -1.0 + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		if (dead) {
			lines.draw_text("You didn't survive 2021 :(",
				glm::vec3(-0.5 * aspect, 0.0, 0.0),
				glm::vec3(0.25, 0.0f, 0.0f), glm::vec3(0.0f, 0.25, 0.0f),
				glm::u8vec4(0xff, 0x00, 0x00, 0x00));
		}

	}
	GL_ERRORS();
}
