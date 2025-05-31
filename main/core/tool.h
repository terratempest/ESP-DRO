#pragma once
#include <string>
#include <vector>

// Represents a single tool and its offsets for all axes
struct Tool {
    std::string name;
    std::vector<float> offsets;

    Tool() = default;

    // Construct with name and offsets vector
    Tool(const std::string& n, const std::vector<float>& offs)
        : name(n), offsets(offs) {}

    // Get offset for given axis index
    float getOffset(int axis) const {
        if (axis >= 0 && axis < (int)offsets.size()) return offsets[axis];
        return 0.0f;
    }

    // Set offset for given axis index
    void setOffset(int axis, float value) {
        if (axis >= 0) {
            if (axis >= (int)offsets.size())
                offsets.resize(axis + 1, 0.0f);
            offsets[axis] = value;
        }
    }
};
