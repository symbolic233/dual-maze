#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include "Level.hpp"
#include "data_path.hpp"

// Some read/write framework inspiration taken from https://15466.courses.cs.cmu.edu/lesson/assets

void write_level(const Level &level, std::ostream *to_) {
    auto &to = *to_;
    to.write(reinterpret_cast<char const*>(&level.rows), 4);
    to.write(reinterpret_cast<char const*>(&level.cols), 4);
    to.write(reinterpret_cast<char const*>(&level.red_start), 4);
    to.write(reinterpret_cast<char const*>(&level.red_end), 4);
    to.write(reinterpret_cast<char const*>(&level.blue_start), 4);
    to.write(reinterpret_cast<char const*>(&level.blue_end), 4);
    to.write(level.walls.data(), level.rows * level.cols);
}

void read_level(std::istream &from, Level *level) {
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
    if (!from.read(reinterpret_cast<char*>(&grid), header.rows * header.cols)) {
		throw std::runtime_error("Failed to read level grid");
	}
    level->walls = std::vector<char>(*grid);
}