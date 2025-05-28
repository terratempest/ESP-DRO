#pragma once
#include <string>

// Represents a single tool and its offsets
struct Tool {
    std::string name;
    float x_offset = 0.0f;
    float z_offset = 0.0f;

    Tool() = default;
    Tool(const std::string& n, float x, float z) : name(n), x_offset(x), z_offset(z) {}
};
