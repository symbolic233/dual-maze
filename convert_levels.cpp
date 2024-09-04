#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include "Level.hpp"
#include "data_path.hpp"

// note: some stream reading inspiration taken from https://stackoverflow.com/questions/7868936/read-file-line-by-line-using-ifstream-in-c
void write_text(const Level &level, std::ostream *to_) {
    auto &to = *to_;
    to.write(reinterpret_cast<char const*>(&level.rows), 4);
    to.write(reinterpret_cast<char const*>(&level.cols), 4);
    to.write(reinterpret_cast<char const*>(&level.red_start), 4);
    to.write(reinterpret_cast<char const*>(&level.red_end), 4);
    to.write(reinterpret_cast<char const*>(&level.blue_start), 4);
    to.write(reinterpret_cast<char const*>(&level.blue_end), 4);
    to.write(level.walls.data(), level.rows * level.cols);
}

int main() {
    uint32_t lnum = 1;
    std::ifstream infile(data_path("../assets/level" + std::to_string(lnum) + ".txt"));
    while (infile.is_open()) {
        Level next;
        infile >> next.rows >> next.cols;
        next.walls.assign(next.rows * next.cols, 0b00);
        uint32_t i = 0;
        while (i < next.rows * next.cols) {
            char c;
            infile.get(c);
            switch (c) {
                case '#':
                    next.walls[i] = 0b01;
                    break;
                case '.':
                    break;
                case 'r':
                    next.red_start = i;
                    break;
                case 'R':
                    next.red_end = i;
                    break;
                case 'b':
                    next.blue_start = i;
                    break;
                case 'B':
                    next.blue_end = i;
                    break;
                case '3':
                    next.red_start = i;
                    next.red_end = i;
                    break;
                case '2':
                    next.blue_start = i;
                    next.red_end = i;
                    break;
                case '1':
                    next.red_start = i;
                    next.blue_end = i;
                    break;
                case '0':
                    next.blue_start = i;
                    next.blue_end = i;
                    break;
                default:
                    i--;
                    break;
            }
            i++;
        }
        std::ofstream writer(data_path("assets/level" + std::to_string(lnum) + ".level"), std::ios::binary);
        write_text(next, &writer);
        writer.close();
        lnum++;
        infile = std::ifstream(data_path("../assets/level" + std::to_string(lnum) + ".txt"));
    }
    std::cout << "Recorded " << lnum - 1 << " levels" << std::endl;
    return 0;
}