#pragma once

#include <entt/entt.hpp>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <stb_image.h>

// data structures.
#include "graphics/Shader.hpp"
#include "Texture.hpp"
#include "Model.hpp"

// ! pain
// generates a loader class with the specified type and loader function.
// send your function as the second parameter.
// the function signature to load files must be the following:
// auto load(const std::string &filepath) -> std::shared_ptr<Texture>
#define CREATE_LOADER_CLASS(type, ...)                                      \
    using type##Cache = entt::resource_cache<type>;                         \
    class type##Loader : public entt::resource_loader<type##Loader, type> { \
    public:                                                                 \
        __VA_ARGS__                                                         \
    private:                                                                \
        type##Cache _cache;                                                 \
    };

// ! suffering
// defines an object instance from the generated class.
#define CREATE_LOADER_INSTANCE(type) type##Loader _##type##Loader;

// ! agony
// use this macro in the cpp file to create a template specialisation
// leading to the global loader class to be able to call youre loader.
#define CREATE_LOADER_CALLER(type)                                                      \
    template<>                                                                          \
    auto kawe::ResourceLoader::load(const std::string &filepath)->std::shared_ptr<type> \
    {                                                                                   \
        return _##type##Loader.load(filepath);                                          \
    }

namespace kawe {

// creates a texture loader.
CREATE_LOADER_CLASS(
    Texture, auto load(const std::string &filepath)->std::shared_ptr<Texture> {
        int width, height, channels;
        auto data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
        if (!data) {
            spdlog::error("couldn't load texture at '{}'.", filepath);
            return nullptr;
        }
        return std::make_shared<Texture>(width, height, channels, data);
    })

// creates a model loader.
CREATE_LOADER_CLASS(
    Model, auto load(const std::string &filepath)->std::shared_ptr<Model> {
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        tinyobj::attrib_t attributes;

        // TODO: reserve the exact amount of space inside vectors.
        auto model = std::make_shared<Model>();
        std::string err;

        bool ret = tinyobj::LoadObj(&attributes, &shapes, &materials, &err, filepath.c_str());

        if (!err.empty()) spdlog::error("{}", err);

        if (!ret) {
            spdlog::error("failed to load model");
            return nullptr;
        }

        // possible using glm/gtx/hash header.
        std::unordered_map<glm::vec3, uint32_t> uniqueVertices;

        // TODO: move this code into the model loader.
        for (const auto &shape : shapes) {
            for (const auto &index : shape.mesh.indices) {
                glm::vec3 position{
                    attributes.vertices[3 * size_t(index.vertex_index) + 0],
                    attributes.vertices[3 * size_t(index.vertex_index) + 1],
                    attributes.vertices[3 * size_t(index.vertex_index) + 2]};

                glm::vec3 normal{glm::vec3(0.0)};
                glm::vec2 texcoord{glm::vec2(0.0)};

                if (index.normal_index >= 0)
                    normal = {
                        attributes.normals[3 * size_t(index.normal_index) + 0],
                        attributes.normals[3 * size_t(index.normal_index) + 1],
                        attributes.normals[3 * size_t(index.normal_index) + 2]};

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

        return model;
    }
)

// creates a texture loader.
CREATE_LOADER_CLASS(
    Shader, auto load(const std::string &filepath)->std::shared_ptr<Shader> {
        auto extension = std::filesystem::path(filepath).extension();

        ShaderType new_shader_type{ShaderType::UNKNOWN};
        std::uint32_t new_shader_type_value = 0;

        // searching for the correct shader type using the file's extension.
        for (const auto &[stype, svalue] : SHADER_TYPES)
            if (extension.string() == fmt::format(".{}", magic_enum::enum_name(stype))) {
                new_shader_type = stype;
                new_shader_type_value = svalue;
                break;
            }

        if (new_shader_type == ShaderType::UNKNOWN) {
            spdlog::warn("failed to load shader at '{}': unknown format.", filepath);
            return nullptr;
        }

        // reading source code.
        std::ifstream shader_file{filepath};

        if (shader_file.is_open()) {
            std::string shader_code {
                (std::istreambuf_iterator<char>(shader_file)), (std::istreambuf_iterator<char>())
            };

            return std::make_shared<Shader>(shader_code.data(), new_shader_type_value);
        }

        return nullptr;
    }
)

// global resource loader class that encapsulate all sub-loaders.
class ResourceLoader {
public:
    template<typename T>
    auto load([[maybe_unused]] const std::string &filepath) -> std::shared_ptr<T>;

private:
    CREATE_LOADER_INSTANCE(Texture)
    CREATE_LOADER_INSTANCE(Shader)
    CREATE_LOADER_INSTANCE(Model)
};
} // namespace kawe
