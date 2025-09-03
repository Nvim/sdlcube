#include "program.h"
#include <SDL3/SDL_gpu.h>

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
  SDL_GPUGraphicsPipeline* pipeline_{nullptr};

private:
  bool LoadShaders();
};
