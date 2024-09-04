#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "data_path.hpp"

// levels are numbered 1-(max)
// tiles can be named, but I want to encode them by single characters in a grid format
// when the two player sprites get to their destinations, immediately go to the next level

#ifndef LEVEL_STRUCT
#define LEVEL_STRUCT

struct Level {
    uint32_t rows;
    uint32_t cols;
    uint32_t red_start;
    uint32_t blue_start;
    uint32_t red_end;
    uint32_t blue_end;
    std::vector<char> walls;
};

typedef std::vector<Level> Levels;

void write_level(const Level &level, std::ofstream to);

void read_level(std::ifstream &from, Level *level);

#endif