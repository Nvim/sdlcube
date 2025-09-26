#include "gltf_loader.h"
#include "logger.h"

#include "fastgltf/glm_element_traits.hpp"
#include "fastgltf/types.hpp"
#include "src/util.h"
#include <SDL3/SDL_surface.h>
#include <cassert>
#include <fastgltf/tools.hpp>
#include <filesystem>
#include <glm/ext/vector_float3.hpp>
#include <string>
#include <variant>

GLTFLoader::GLTFLoader(std::filesystem::path path)
  : path_{ path }
{
}

GLTFLoader::~GLTFLoader()
{
  for (SDL_Surface* s : images_) {
    SDL_DestroySurface(s);
  }
}

const std::vector<MeshAsset>&
GLTFLoader::Meshes() const
{
  return meshes_;
}

const std::vector<SDL_Surface*>&
GLTFLoader::Surfaces() const
{
  return images_;
}

bool
GLTFLoader::Load()
{
  if (!std::filesystem::exists(path_)) {
    LOG_ERROR("path {} is invalid", path_.c_str());
    return false;
  }
  LOG_TRACE("LoadMesh");
  auto data = fastgltf::GltfDataBuffer::FromPath(path_);
  if (data.error() != fastgltf::Error::None) {
    LOG_ERROR("couldn't load gltf from path");
    return false;
  }

  constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;

  fastgltf::Asset gltf;
  fastgltf::Parser parser{};

  auto asset = parser.loadGltf(data.get(), path_.parent_path(), gltfOptions);
  // auto asset =
  //   parser.loadGltfBinary(data.get(), path_.parent_path(), gltfOptions);
  if (auto error = asset.error(); error != fastgltf::Error::None) {
    LOG_ERROR("couldn't parse binary gltf");
    return false;
  }

  if (auto error = fastgltf::validate(asset.get());
      error != fastgltf::Error::None) {
    LOG_ERROR("couldn't validate gltf");
    return false;
  }

  asset_ = std::move(asset.get());

  loaded_ = LoadVertexData();
  if (!loaded_) {
    LOG_ERROR("Couldn't load vertex data from GLTF");
    return false;
  }
  LOG_DEBUG("Loaded GLTF meshes");
  loaded_ = LoadImageData();
  if (!loaded_) {
    LOG_ERROR("Couldn't load images from GLTF");
    return false;
  }
  LOG_DEBUG("Loaded GLTF images");

  return loaded_;
}

bool
GLTFLoader::LoadVertexData()
{
  LOG_TRACE("LoadVertexData");
  if (asset_.meshes.empty()) {
    LOG_WARN("LoadVertexData: GLTF has no meshes");
    return false;
  }
  // construct our Mesh vector at the same time we fill vertices and indices
  // buffers
  meshes_ = std::vector<MeshAsset>{};
  meshes_.reserve(asset_.meshes.size());

  for (auto& mesh : asset_.meshes) {
    MeshAsset newMesh;
    newMesh.Name = mesh.name.c_str();
    auto& indices = newMesh.indices_;
    auto& vertices = newMesh.vertices_;

    for (auto&& p : mesh.primitives) {
      Geometry newGeometry{
        .FirstIndex = indices.size(),
        .VertexCount = (u32)asset_.accessors[p.indicesAccessor.value()].count
      };

      size_t initial_vtx = vertices.size();

      { // load indexes
        fastgltf::Accessor& indexaccessor =
          asset_.accessors[p.indicesAccessor.value()];
        indices.reserve(indices.size() + indexaccessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(
          asset_, indexaccessor, [&](std::uint32_t idx) {
            indices.push_back(idx + initial_vtx);
          });
      }

      { // load vertex positions
        fastgltf::Accessor& posAccessor =
          asset_.accessors[p.findAttribute("POSITION")->accessorIndex];
        vertices.resize(vertices.size() + posAccessor.count);

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
          asset_, posAccessor, [&](glm::vec3 v, size_t index) {
            PosUvVertex newvtx;
            newvtx.pos[0] = v.x;
            newvtx.pos[1] = v.y;
            newvtx.pos[2] = v.z;
            newvtx.uv[0] = 0;
            newvtx.uv[1] = 0;
            vertices[initial_vtx + index] = newvtx;
          });
      }

      { // load vertex UVs
        auto attr = p.findAttribute("TEXCOORD_0");
        if (attr != p.attributes.end()) {
          fastgltf::iterateAccessorWithIndex<glm::vec2>(
            asset_,
            asset_.accessors[(*attr).accessorIndex],
            [&](glm::vec2 v, size_t index) {
              vertices[initial_vtx + index].uv[0] = v.x;
              vertices[initial_vtx + index].uv[1] = v.y;
            });
        }
      }

      newMesh.Submeshes.push_back(newGeometry);
      LOG_DEBUG("New geometry. Total Verts: {}, Total Indices: {} || {}, {}",
                vertices.size(),
                indices.size(),
                newMesh.Submeshes[0].FirstIndex,
                newMesh.Submeshes[0].VertexCount);
    }
    meshes_.emplace_back(newMesh);
    assert(newMesh.indices_.size() != 0);
  }
  assert(meshes_[0].indices_.size() != 0);
  return true;
}

bool
GLTFLoader::LoadImageData()
{
  LOG_TRACE("LoadImageData");
  if (asset_.images.empty()) {
    LOG_WARN("LoadImageData: GLTF has no images");
    return false;
  }

  images_ = std::vector<SDL_Surface*>{};
  // images_.reserve(asset_.images.size());

  u32 tex_idx =
    asset_.materials[0].pbrData.baseColorTexture.value().textureIndex;
  assert(tex_idx == 0);
  for (auto& image : asset_.images) {
    LOG_TRACE("Visiting image");
    // clang-format off
    // std::visit(fastgltf::visitor{ 
    //   []([[maybe_unused]] auto& arg) {LOG_WARN("No URI source");},
    //   [&](fastgltf::sources::URI& filePath) {
    //     assert(filePath.fileByteOffset == 0);
    //     assert(filePath.uri.isLocalPath());
    //     const std::string path(
    //       filePath.uri.path().begin(),
    //       filePath.uri.path().end()
    //     );
    //     auto surface = LoadImage(path.c_str());
    //     if (!surface) {
    //       LOG_ERROR("Couldn't load image: {}", SDL_GetError());
    //     }
    //     // else {
    //       images_.push_back(surface);
    //     // }
    //   }
    // }, image.data);
    // clang-format on
    if (std::holds_alternative<fastgltf::sources::URI>(image.data)) {
      LOG_DEBUG("image has an URI. Index: {}", image.data.index());
      // if (image.data.index() != 0) {
      //   continue;
      // }
      LOG_DEBUG("found the 0th image");
      auto filePath = std::get<fastgltf::sources::URI>(image.data);
      assert(filePath.fileByteOffset == 0);
      assert(filePath.uri.isLocalPath());
      const std::string path(filePath.uri.path().begin(),
                             filePath.uri.path().end());
      auto* surface = LoadImage(
        std::format("{}{}", "resources/models/BarramundiFishGLTF/", path)
          .c_str());
      if (!surface) {
        LOG_ERROR("Couldn't load image: {}", SDL_GetError());
      } else {
        images_.push_back(surface);
      }
    } else {
      LOG_WARN("Image doesn't have an URI");
    }
  }

  LOG_DEBUG("{} images were pushed", images_.size());
  return !images_.empty();
}
