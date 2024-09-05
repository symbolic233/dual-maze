#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>
#include <iostream>
#include "data_path.hpp"
#include "Load.hpp"
#include "Level.hpp"

/*Load<Levels> levels(LoadTagDefault, [&](){
	Levels *ret;

});*/

static Level current_lvl;

void read_level_stream(std::istream &from, Level *level) {
    struct LevelHeader {
		uint32_t rows;
        uint32_t cols;
        uint32_t red_start;
		uint32_t red_end;
        uint32_t blue_start;
        uint32_t blue_end;
	};
    LevelHeader header;
    if (!from.read(reinterpret_cast<char*>(&header), sizeof(header))) {
		throw std::runtime_error("Failed to read level header");
	}
    level->rows = header.rows;
    level->cols = header.cols;
    level->red_start = header.red_start;
    level->blue_start = header.blue_start;
    level->red_end = header.red_end;
    level->blue_end = header.blue_end;
    char *grid = new char[header.rows * header.cols];
    if (!from.read(grid, header.rows * header.cols)) {
		throw std::runtime_error("Failed to read level grid");
	}
	level->walls.insert(level->walls.begin(), &grid[0], &grid[header.rows * header.cols]);
}

void load_level(Level &l, uint32_t i) {
	std::ifstream place(data_path("assets/level" + std::to_string(i) + ".level"), std::ios_base::binary);
	read_level_stream(place, &l);
}

void PlayMode::update_level(uint32_t i) {
	load_level(current_lvl, i);
	// let's build the background now!
	// black it out
	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			// palette 1, tile 0
			ppu.background[x + PPU466::BackgroundWidth * y] = (1 << 8) + 0;
		}
	}
	uint32_t start_x = PPU466::BackgroundWidth / 4 - current_lvl.cols / 2;
	uint32_t start_y = PPU466::BackgroundHeight / 4 - current_lvl.rows / 2;
	for (uint32_t y = 0; y < current_lvl.rows; ++y) {
		for (uint32_t x = 0; x < current_lvl.cols; ++x) {
			uint32_t xpos = start_x + x;
			uint32_t ypos = start_y + y;
			// wall: palette 0, tile 1
			// empty: palette 1, tile 0
			uint16_t color = current_lvl.walls[y * current_lvl.cols + x] ? ((0 << 8) + 1) : ((1 << 8) + 0);
			ppu.background[xpos + PPU466::BackgroundWidth * ypos] = color;
		}
	}
	uint32_t p1x = current_lvl.red_start % current_lvl.cols;
	uint32_t p1y = current_lvl.red_start / current_lvl.cols;
	uint32_t p2x = current_lvl.blue_start % current_lvl.cols;
	uint32_t p2y = current_lvl.blue_start / current_lvl.cols;

	player1_at.x = 8.0f * (start_x + p1x);
	player1_at.y = 8.0f * (start_y + p1y);
	player2_at.x = 8.0f * (start_x + p2x);
	player2_at.y = 8.0f * (start_y + p2y);

	p1.position = current_lvl.red_start;
	p2.position = current_lvl.blue_start;

	uint32_t g1x = current_lvl.red_end % current_lvl.cols;
	uint32_t g1y = current_lvl.red_end / current_lvl.cols;
	uint32_t g2x = current_lvl.blue_end % current_lvl.cols;
	uint32_t g2y = current_lvl.blue_end / current_lvl.cols;

	uint32_t place1 = (start_x + g1x) + PPU466::BackgroundWidth * (start_y + g1y);
	uint32_t place2 = (start_x + g2x) + PPU466::BackgroundWidth * (start_y + g2y);
	// red goal: palette 4, tile 4
	// blue goal: palette 2, tile 5
	ppu.background[place1] = (4 << 8) + 4;
	ppu.background[place2] = (2 << 8) + 5;

	p1.goal = current_lvl.red_end;
	p2.goal = current_lvl.blue_end;
	assert(!current_lvl.walls[p1.position]);
	assert(!current_lvl.walls[p1.goal]);
	assert(!current_lvl.walls[p2.position]);
	assert(!current_lvl.walls[p2.goal]);
}

PlayMode::PlayMode() {
	// black BG
	ppu.background_color = glm::u8vec4(
		0x00,
		0x00,
		0x00,
		0xff
	);

	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	// make yourself a script that spits out the code that you paste in here
	// and check that script into your repository.
	std::string tileset_filename = data_path("assets/tile_table.bin");
	std::ifstream tileset_file(tileset_filename, std::ios_base::binary);
	tileset_file.read(reinterpret_cast<char*> (ppu.tile_table.data()), sizeof(ppu.tile_table));

	std::string palette_filename = data_path("assets/palette_table.bin");
	std::ifstream palette_file(palette_filename, std::ios_base::binary);
	palette_file.read(reinterpret_cast<char*> (ppu.palette_table.data()), sizeof(ppu.palette_table));

	std::string bg_filename = data_path("assets/background.bin");
	std::ifstream bg_file(bg_filename, std::ios_base::binary);
	bg_file.read(reinterpret_cast<char*> (ppu.background.data()), sizeof(ppu.background));

	update_level(lnum);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	constexpr float TileChange = 8.0f;
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			char wall1 = current_lvl.walls[p1.position - 1];
			char wall2 = current_lvl.walls[p2.position - 1];
			bool coll1 = (p1.position - 1 == p2.position);
			bool coll2 = (p2.position - 1 == p1.position);
			if (!wall1 && (!coll1 || !wall2)) {
				player1_at.x -= TileChange;
				p1.position -= 1;
			}
			if (!wall2 && (!coll2 || !wall1)) {
				player2_at.x -= TileChange;
				p2.position -= 1;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			char wall1 = current_lvl.walls[p1.position + 1];
			char wall2 = current_lvl.walls[p2.position + 1];
			bool coll1 = (p1.position + 1 == p2.position);
			bool coll2 = (p2.position + 1 == p1.position);
			if (!wall1 && (!coll1 || !wall2)) {
				player1_at.x += TileChange;
				p1.position += 1;
			}
			if (!wall2 && (!coll2 || !wall1)) {
				player2_at.x += TileChange;
				p2.position += 1;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			char wall1 = current_lvl.walls[p1.position + current_lvl.cols];
			char wall2 = current_lvl.walls[p2.position + current_lvl.cols];
			bool coll1 = (p1.position + current_lvl.cols == p2.position);
			bool coll2 = (p2.position + current_lvl.cols == p1.position);
			if (!wall1 && (!coll1 || !wall2)) {
				player1_at.y += TileChange;
				p1.position += current_lvl.cols;
			}
			if (!wall2 && (!coll2 || !wall1)) {
				player2_at.y += TileChange;
				p2.position += current_lvl.cols;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			char wall1 = current_lvl.walls[p1.position - current_lvl.cols];
			char wall2 = current_lvl.walls[p2.position - current_lvl.cols];
			bool coll1 = (p1.position - current_lvl.cols == p2.position);
			bool coll2 = (p2.position - current_lvl.cols == p1.position);
			if (!wall1 && (!coll1 || !wall2)) {
				player1_at.y -= TileChange;
				p1.position -= current_lvl.cols;
			}
			if (!wall2 && (!coll2 || !wall1)) {
				player2_at.y -= TileChange;
				p2.position -= current_lvl.cols;
			}
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	// constexpr float TileChange = 8.0f;
	
	/*if (left.pressed) {
		player1_at.x -= TileChange;
		player2_at.x -= TileChange;
	}
	if (right.pressed) {
		player1_at.x += TileChange;
		player2_at.x += TileChange;
	}
	if (down.pressed) {
		player1_at.y -= TileChange;
		player2_at.y -= TileChange;
	}
	if (up.pressed) {
		player1_at.y += TileChange;
		player2_at.y += TileChange;
	}*/

	if ((p1.position == p1.goal) && (p2.position == p2.goal) && lnum != max_levels) {
		lnum++;
		update_level(lnum);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	//player sprite 1:
	ppu.sprites[0].x = int8_t(player1_at.x);
	ppu.sprites[0].y = int8_t(player1_at.y);
	ppu.sprites[0].index = 2;
	ppu.sprites[0].attributes = 5;

	//player sprite 2:
	ppu.sprites[1].x = int8_t(player2_at.x);
	ppu.sprites[1].y = int8_t(player2_at.y);
	ppu.sprites[1].index = 3;
	ppu.sprites[1].attributes = 3;

	//--- actually draw ---
	ppu.draw(drawable_size);
}
