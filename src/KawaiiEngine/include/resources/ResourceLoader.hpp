#pragma once

#include <entt/entt.hpp>
#include "tiny_obj_loader.h"
#include "Texture.hpp"

#define TINYOBJLOADER_IMPLEMENTATION

// generates a loader class with the specified type and loader function.
// send your function as the second parameter.
#define CREATE_LOADER_CLASS(type, ...)                                              \
    using type ## Cache = entt::resource_cache<type>;                               \
    class type ## Loader : public entt::resource_loader<type ## Loader, type> {     \
    public:                                                                         \
        __VA_ARGS__                                                                 \
    private:                                                                        \
        type ## Cache _cache;                                                       \
    };

// defines an object instance from the generated class.
#define CREATE_LOADER_INSTANCE(type) type ## Loader _type ## Loader;

namespace kawe {

    CREATE_LOADER_CLASS(Texture,
        auto load_texture(const std::string &filepath) const -> std::shared_ptr<Texture> {
            int width, height, channels;
            auto data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
            if (!data) {
                spdlog::error("couldn't load texture at '%s'.", filepath);
                return nullptr;
            }
            return std::make_shared<Texture>(
                Texture {
                    width, height, channels, data
                }
            );
        }
    )

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
        CREATE_LOADER_INSTANCE(Texture)
    };
}