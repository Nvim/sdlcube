#include "gltf_loader.h"
#include "logger.h"

#include "fastgltf/glm_element_traits.hpp"
#include "fastgltf/types.hpp"
#include "src/util.h"
#include <fastgltf/tools.hpp>
#include <filesystem>
#include <glm/ext/vector_float3.hpp>

GLTFLoader::GLTFLoader(std::filesystem::path path)
  : path_{ path }
{
}
const std::vector<MeshAsset>&
GLTFLoader::Meshes() const
{
  return meshes_;
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

  auto asset =
    parser.loadGltfBinary(data.get(), path_.parent_path(), gltfOptions);
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

  return loaded_;
}

bool
GLTFLoader::LoadVertexData()
{
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
            PosVertex newvtx;
            newvtx.pos[0] = v.x;
            newvtx.pos[1] = v.y;
            newvtx.pos[2] = v.z;
            vertices[initial_vtx + index] = newvtx;
          });
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
