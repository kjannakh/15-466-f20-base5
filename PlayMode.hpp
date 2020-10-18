#include "Mode.hpp"

#include "Scene.hpp"
#include "WalkMesh.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <random>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const&, glm::uvec2 const& window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const& drawable_size) override;

	//camera update code
	bool looking = false;

	float syringe_pitch = 0.0f;
	float syringe_tilt_rate = 20.0f;
	float syringe_max_tilt = 30.0f;
	float syringe_min_tilt = -15.0f;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space, shift, mouse;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//player info:
	struct Player {
		WalkPoint at;
		//transform is at player's feet and will be yawed by mouse left/right motion:
		Scene::Transform* transform = nullptr;
		//camera is at player's head and will be pitched by mouse up/down motion:
		Scene::Camera* camera = nullptr;
	} player;
	Scene::Transform* syringe = nullptr;
	Scene::Transform* tip = nullptr;

	static const int num_viruses = 5;
	enum VirusState { DEAD, ALIVE };
	struct Virus {
		VirusState state = DEAD;
		Scene::Transform* transform = nullptr;
		float speed = 1.0f;
		glm::vec3 direction;
		glm::vec3 rotate_axis;
		int dlat;
		int dlong;
	};
	struct Virus viruses[num_viruses];
	const char* colors[5] = {"Purple", "Pink", "Red", "Orange", "Yellow"};

	float spawn_distance = 50.0f;
	int spawn_num = 0;
	std::mt19937 mt;
	float virus_speed = 1.5f;
	float virus_max_speed = 5.0f;
	float virus_spawn_timer = 0.0f;
	float virus_spawn_rate = 8.0f;
	float min_spawn_rate = 3.0f;
	float virus_speed_timer = 0.0f;
	float virus_speed_rate = 20.0f;
	float virus_speed_increase = 0.5f;
	float virus_spawn_rate_decrease = 0.2f;

	float player_speed = 30.0f;
	float player_speed_increase = 2.5f;
	float player_max_speed = 60.0f;

	float game_timer = 0.0f;
	int high_score = 0;

	float neutralize_dist = 4.0f;

	int lives = 5;
	bool reset_game = false;

	bool dead = false;
	float dead_timer = 0.0f;
	float dead_rate = 3.0f;

	float draw_spacing = 1.5f;

	int player_lat = 0;
	int player_long = 0;

	void spawn_virus();
	void update_viruses(float elapsed);
	bool tip_near_virus();
	void neutralize();
	void kill_virus(int i);
	void resetGame();
};
