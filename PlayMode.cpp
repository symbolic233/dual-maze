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

void read_level_stream(std::istream &from, Level *level) {
    struct LevelHeader {
		uint32_t rows;
        uint32_t cols;
        uint32_t red_start;
        uint32_t blue_start;
        uint32_t red_end;
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

	player1_at.x = PPU466::ScreenWidth / 2 - 8;
	player1_at.y = PPU466::ScreenHeight / 2 - 8;
	player2_at.x = PPU466::ScreenWidth / 2;
	player2_at.y = PPU466::ScreenHeight / 2;

	std::string bg_filename = data_path("assets/background.bin");
	std::ifstream bg_file(bg_filename, std::ios_base::binary);
	bg_file.read(reinterpret_cast<char*> (ppu.background.data()), sizeof(ppu.background));

	Level next;
	load_level(next, 1);
	// let's build the background now!
	// black it out
	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			// palette 1, tile 0
			ppu.background[x+PPU466::BackgroundWidth*y] = (1 << 8) + 0;
		}
	}
	uint32_t start_x = PPU466::BackgroundWidth / 16 - next.cols / 2;
	uint32_t start_y = PPU466::BackgroundHeight / 16 - next.rows / 2;
	// let's build the background now!
	for (uint32_t y = 0; y < next.cols; ++y) {
		for (uint32_t x = 0; x < next.rows; ++x) {
			uint32_t xpos = start_x + x;
			uint32_t ypos = start_y + y;
			// wall: palette 0, tile 1
			// empty: palette 1, tile 0
			uint16_t color = next.walls[y * next.cols + x] ? ((0 << 8) + 1) : ((1 << 8) + 0);
			std::cout << color << std::endl;
			ppu.background[xpos + PPU466::BackgroundWidth * ypos] = color;
		}
	}

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	constexpr float TileChange = 8.0f;
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			player1_at.x -= TileChange;
			player2_at.x -= TileChange;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			player1_at.x += TileChange;
			player2_at.x += TileChange;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			player1_at.y += TileChange;
			player2_at.y += TileChange;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			player1_at.y -= TileChange;
			player2_at.y -= TileChange;
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

	//some other misc sprites:
	/*
	for (uint32_t i = 1; i < 63; ++i) {
		float amt = (i + 2.0f * background_fade) / 62.0f;
		ppu.sprites[i].x = int8_t(0.5f * PPU466::ScreenWidth + std::cos( 2.0f * M_PI * amt * 5.0f + 0.01f * player_at.x) * 0.4f * PPU466::ScreenWidth);
		ppu.sprites[i].y = int8_t(0.5f * PPU466::ScreenHeight + std::sin( 2.0f * M_PI * amt * 3.0f + 0.01f * player_at.y) * 0.4f * PPU466::ScreenWidth);
		ppu.sprites[i].index = 32;
		ppu.sprites[i].attributes = 6;
		if (i % 2) ppu.sprites[i].attributes |= 0x80; //'behind' bit
	}
	*/

	//--- actually draw ---
	ppu.draw(drawable_size);
}
