#pragma once

#include "src/util.h"
#include "types.h"
#include <fastgltf/core.hpp>
#include <vector>

struct Geometry {
  const std::size_t FirstIndex;
  const std::size_t VertexCount;
};

struct MeshAsset{
  const char* Name;
  std::vector<Geometry> Submeshes;

  // TODO: don't duplicate these, send them to GPU directly when loading
  std::vector<PosVertex> vertices_{};
  std::vector<u32> indices_{};
};

class GLTFLoader {
public:
  GLTFLoader(std::filesystem::path path);
  ~GLTFLoader() = default;

  bool Load();
  const std::vector<MeshAsset>& Meshes() const;

private:
  bool LoadVertexData();

private:
  fastgltf::Asset asset_;
  std::filesystem::path path_;
  bool loaded_{false};

  // Geometry stuff:
  std::vector<MeshAsset> meshes_;
};
