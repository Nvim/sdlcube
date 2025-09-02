#include "program.h"

class CubeProgram : public Program {
public:
  CubeProgram(SDL_GPUDevice* device, SDL_Window* Window);
  bool Init() override;
  bool Poll() override;
  bool Draw() override;
  bool ShouldQuit() override;

private:
  bool quit{false};
};
