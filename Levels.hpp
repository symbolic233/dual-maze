#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <map>

// levels are numbered 1-(max)
// tiles can be named, but I want to encode them by single characters in a grid format
// when the two player sprites get to their destinations, immediately go to the next level

// VSC complaints
template< typename T >
T const *new_T() { return new T; }

template< typename T >
struct Levels {
	enum : uint8_t {
		Wall = 0x80,
		RedStart = 0x10,
        RedEnd = 0x11,
        BlueStart = 0x20,
        BlueEnd = 0x21,
        Empty = 0x00
	};

	struct Level {
        glm::uvec2 size = glm::uvec2(0U,0U);
        std::vector<uint8_t> tiles;
    }

	std::vector<Level> levels;

	Level const &lookup(uint8_t index);
};