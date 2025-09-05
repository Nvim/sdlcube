#include "program.h"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>

typedef struct Vertex {
  float pos[3];
} Vertex;

class CubeProgram : public Program {
public:
  CubeProgram(SDL_GPUDevice *device, SDL_Window *window,
              const char *vertex_path, const char *fragment_path);
  bool Init() override;
  bool Poll() override;
  bool Draw() override;
  bool ShouldQuit() override;
  ~CubeProgram();

private:
  bool quit{false};
  const char *vertex_path_;
  const char *fragment_path_;
  SDL_GPUShader *vertex_{nullptr};
  SDL_GPUShader *fragment_{nullptr};
  SDL_GPUGraphicsPipeline *pipeline_{nullptr};
  bool SendVertexData();
  SDL_GPUBuffer *vbuffer_;
  SDL_GPUBuffer *ibuffer_;

  // clang-format off
  static constexpr Uint8 VERT_COUNT = 4;
  static constexpr Vertex verts[VERT_COUNT] = {
    {-.6f,  .6f, 0},
    { .6f,  .6f, 0},
    { .6f, -.6f, 0},
    {-.6f, -.6f, 0},
  };

  static constexpr Uint8 INDEX_COUNT = 6;
  static constexpr Uint16 indices[INDEX_COUNT] = {
    0, 1, 2,
    0, 2, 3,
  };
  // clang-format on

private:
  bool LoadShaders();
};
