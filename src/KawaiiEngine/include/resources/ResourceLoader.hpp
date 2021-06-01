#pragma once

#include <entt/entt.hpp>
#include <spdlog/spdlog.h>
#include <glm/gtx/hash.hpp>

#include "stb_image.h"

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
    auto kawe::ResourceLoader::load(const std::string &filepath) -> std::shared_ptr<type> {             \
        return _ ## type ## Loader.load(filepath);                                                      \
    }

namespace kawe {

    // creates a texture loader.
    CREATE_LOADER_CLASS(Texture,
        auto load(const std::string &filepath) -> std::shared_ptr<Texture> {
            int width, height, channels;
            auto data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
            if (!data) {
                spdlog::error("couldn't load texture at '{}'.", filepath);
                return nullptr;
            }
            return std::make_shared<Texture>(
                Texture {
                    width, height, channels, data
                }
            );
        }
    )

    // creates a model loader.
    CREATE_LOADER_CLASS(Model,
        auto load(const std::string &filepath) -> std::shared_ptr<Model> {

            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            tinyobj::attrib_t attributes;

            // TODO: reserve the exact amount of space inside vectors.
            auto model = std::make_shared<Model>();
            std::string err;

            bool ret = tinyobj::LoadObj(&attributes, &shapes, &materials, &err, filepath.c_str());

            if (!err.empty())
                spdlog::error("{}", err);

            if (!ret) {
                spdlog::error("failed to load model");
                return nullptr;
            }

            // possible using glm/gtx/hash header.
            std::unordered_map<glm::vec3, uint32_t> uniqueVertices;

            // TODO: move this code into the model loader.
            for (const auto &shape : shapes) {
                for (const auto &index : shape.mesh.indices) {

                    glm::vec3 position {
                        attributes.vertices[3 * size_t(index.vertex_index) + 0],
                        attributes.vertices[3 * size_t(index.vertex_index) + 1],
                        attributes.vertices[3 * size_t(index.vertex_index) + 2]
                    };

                    glm::vec3 normal{glm::vec3(0.0)};
                    glm::vec2 texcoord{glm::vec2(0.0)};

                    if (index.normal_index >= 0)
                        normal = {
                            attributes.normals[3 * size_t(index.normal_index) + 0],
                            attributes.normals[3 * size_t(index.normal_index) + 1],
                            attributes.normals[3 * size_t(index.normal_index) + 2]
                        };

                    if (index.texcoord_index >= 0)
                        texcoord = {
                            attributes.texcoords[2 * size_t(index.texcoord_index) + 0],
                            attributes.texcoords[2 * size_t(index.texcoord_index) + 1],
                        };

                    if (!uniqueVertices.contains(position)) {
                        uniqueVertices[position] = static_cast<uint32_t>(model->vertices.size() / 3);

                        // ! not really clean.
                        // TODO: find a better way to store vertices.
                        model->vertices.push_back(position.x);
                        model->vertices.push_back(position.y);
                        model->vertices.push_back(position.z);

                        model->normals.push_back(normal);
                        model->texcoords.push_back(texcoord);
                    }

                    model->indices.push_back(uniqueVertices[position]);
                }
            }

            model->filepath = filepath;

            spdlog::info("model at '{}' loaded successfully.", filepath);
            return model;
        }
    )

    // global resource loader class that encapsulate all sub-loaders.
    class ResourceLoader {
    public:

        ResourceLoader() = default;
        ~ResourceLoader() = default;

        template<typename T>
        auto load([[maybe_unused]] const std::string &filepath) -> std::shared_ptr<T>;

    private:
        CREATE_LOADER_INSTANCE(Texture)
        CREATE_LOADER_INSTANCE(Model)
    };
}