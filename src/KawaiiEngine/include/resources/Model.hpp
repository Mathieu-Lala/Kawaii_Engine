#pragma once

#include "tiny_obj_loader.h"

namespace kawe {
    struct Model {

        Model() = default;
        ~Model() = default;

        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        tinyobj::attrib_t attrib;
    };
}