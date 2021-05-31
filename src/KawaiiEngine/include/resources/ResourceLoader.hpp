#pragma once

#include <entt/entt.hpp>
#include <spdlog/spdlog.h>

#define TINYOBJLOADER_IMPLEMENTATION

// data structures.
#include "Texture.hpp"
#include "Model.hpp"

// ! pain
// generates a loader class with the specified type and loader function.
// send your function as the second parameter.
// the function signature to load files must be the following:
// auto load(const std::string &filepath) -> std::shared_ptr<Texture>
#define CREATE_LOADER_CLASS(type, ...)                                              \
    using type ## Cache = entt::resource_cache<type>;                               \
    class type ## Loader : public entt::resource_loader<type ## Loader, type> {     \
    public:                                                                         \
        __VA_ARGS__                                                                 \
    private:                                                                        \
        type ## Cache _cache;                                                       \
    };

// ! suffering
// defines an object instance from the generated class.
#define CREATE_LOADER_INSTANCE(type) type ## Loader _ ## type ## Loader;

// ! agony
// use this macro in the cpp file to create a template specialisation
// leading to the global loader class to be able to call youre loader.
#define CREATE_LOADER_CALLER(type)                                                                      \
    template<>                                                                                          \
    auto kawe::ResourceLoader::load(const std::string &filepath) -> std::shared_ptr<type> {   \
        return _ ## type ## Loader.load(filepath);                                                      \
    }

namespace kawe {

    // creates a texture loader.
    CREATE_LOADER_CLASS(Texture,
        auto load(const std::string &filepath) -> std::shared_ptr<Texture> {
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

    CREATE_LOADER_CLASS(Model,
        auto load(const std::string &filepath) -> std::shared_ptr<Model> {
            Model model {};
            std::string err;

            bool ret = tinyobj::LoadObj(&model.attrib, &model.shapes, &model.materials, &err, filepath.c_str());

            if (!err.empty())
                spdlog::error("%s", err);
            if (!ret)
                return nullptr;

            return std::make_shared<Model>(model);
        }
    )

    // global resource loader class that encapsulate all sub-loaders.
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
        CREATE_LOADER_INSTANCE(Model)
    };
}