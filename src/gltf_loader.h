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
  std::vector<PosUvVertex> vertices_{};
  std::vector<u32> indices_{};
};

class GLTFLoader {
public:
  GLTFLoader(std::filesystem::path path);
  ~GLTFLoader();

  bool Load();
  const std::vector<MeshAsset>& Meshes() const;
  const std::vector<SDL_Surface*>& Surfaces() const;

private:
  bool LoadVertexData();
  bool LoadImageData();

private:
  fastgltf::Asset asset_;
  std::filesystem::path path_;
  bool loaded_{false};

  std::vector<MeshAsset> meshes_;
  std::vector<SDL_Surface*> images_;
};
