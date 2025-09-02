#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

// Program uses SDL GPU and Window to draw, but isn't responsible for
// creating/releasing them.
class Program {
public:
  SDL_GPUDevice *Device;
  SDL_Window *Window;

public:
  Program(SDL_GPUDevice *device, SDL_Window *window)
      : Device{device}, Window{window} {};
  virtual bool Init() = 0;
  virtual bool Poll() = 0;
  virtual bool Draw() = 0;
  virtual bool ShouldQuit() = 0;
};
