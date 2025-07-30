
#include "file_manager.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

std::string read_file_to_string(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
