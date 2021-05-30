#pragma once

#include <entt/entt.hpp>
#include "tiny_obj_loader.h"
#include "Texture.hpp"

#define TINYOBJLOADER_IMPLEMENTATION

namespace kawe {
    class ResourceLoader {
    public:

        ResourceLoader() = default;
        ~ResourceLoader() = default;

    private:
    };

    using TextureCache = entt::resource_cache<Texture>;
    class TextureLoader : public entt::resource_loader<TextureLoader, Texture> {
        
    };
}