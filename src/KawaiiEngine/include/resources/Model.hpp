#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "tiny_obj_loader.h"

namespace kawe {

struct Model {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texcoords;
    std::vector<uint32_t> indices;
    std::string filepath;
};

} // namespace kawe
