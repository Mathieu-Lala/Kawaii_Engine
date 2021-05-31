#include "resources/ResourceLoader.hpp"

template<>
auto kawe::ResourceLoader::load(const std::string &filepath) -> std::shared_ptr<Texture> {
    return _textureLoader.load(filepath);
}
