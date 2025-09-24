#pragma once

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include "fastgltf/types.hpp"
#include "logger.h"

bool
LoadMesh(std::filesystem::path filePath)
{
  LOG_TRACE("LoadMesh");
  auto data = fastgltf::GltfDataBuffer::FromPath(filePath);
  if (data.error() != fastgltf::Error::None) {
    LOG_ERROR("couldn't load gltf from path");
    return false;
  }

  constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;

  fastgltf::Asset gltf;
  fastgltf::Parser parser{};

  auto asset =  
    parser.loadGltfBinary(data.get(), filePath.parent_path(), gltfOptions);
  if (auto error = asset.error(); error != fastgltf::Error::None) {
    LOG_ERROR("couldn't parse binary gltf");
    return false;
  }
  
  if(auto error = fastgltf::validate(asset.get()); error != fastgltf::Error::None) {
    LOG_ERROR("couldn't validate gltf" );
    return false;
  }

  auto& meshes = asset.get().meshes;
  LOG_DEBUG("{} meshes", meshes.size());
  for(auto& mesh : meshes) {
    LOG_DEBUG("Mesh {}", mesh.name);

    auto& primitives = mesh.primitives;
    for (auto& primitive : primitives) {
      if(primitive.type != fastgltf::PrimitiveType::Triangles) {
        LOG_DEBUG("primitive type isn't triangle");
      }
    }
  }

  return true;
}
