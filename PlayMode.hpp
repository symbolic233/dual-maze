#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
		uint8_t previous = 0;
	} left, right, down, up;

	//player positions:
	glm::vec2 player1_at = glm::vec2(0.0f);
	glm::vec2 player2_at = glm::vec2(0.0f);

	uint32_t current_level = 1;

	struct Player {
		uint32_t position = 0;
		uint32_t goal = 0;
	} p1, p2;

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};
