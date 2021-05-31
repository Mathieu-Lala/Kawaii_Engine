#pragma once

#include <entt/entt.hpp>
#include "tiny_obj_loader.h"
#include "Texture.hpp"

#define TINYOBJLOADER_IMPLEMENTATION

namespace kawe {
    using TextureCache = entt::resource_cache<Texture>;
    class TextureLoader : public entt::resource_loader<TextureLoader, Texture> {
    public:
        auto load(const std::string &filepath) const -> std::shared_ptr<Texture> {

            // TODO: check the cache before loading a new file in memory.
            auto texture = std::make_shared<Texture>(Texture{ filepath });

            return texture->is_initialised() ? texture : nullptr;
        }
    private:
        TextureCache _cache;
    };

    // global resource loader class that encapsulate all
    // the sub-loaders.
    class ResourceLoader {
    public:

        ResourceLoader() = default;
        ~ResourceLoader() = default;

        template<typename T>
        auto load(const std::string &filepath) -> std::shared_ptr<T> {
            return nullptr;
        }

    private:
        TextureLoader _textureLoader;
    };
}