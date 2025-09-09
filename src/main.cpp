#include "cube.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_video.h>

int main() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
    return 1;
  }

  SDL_Log("Initialized SDL");

  auto Device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV |
                                        SDL_GPU_SHADERFORMAT_DXIL |
                                        SDL_GPU_SHADERFORMAT_MSL,
                                    true, NULL);

  if (Device == NULL) {
    SDL_Log("GPUCreateDevice failed");
    return -1;
  }
  SDL_Log("Created Device");

  auto Window =
      SDL_CreateWindow("Some cube idk", 640, 480, SDL_WINDOW_RESIZABLE);
  if (Window == NULL) {
    SDL_Log("CreateWindow failed: %s", SDL_GetError());
    return -1;
  }
  SDL_Log("Created Window");

  if (!SDL_ClaimWindowForGPUDevice(Device, Window)) {
    SDL_Log("GPUClaimWindow failed");
    return -1;
  }
  SDL_Log("GPU claimed Window");

  CubeProgram app{Device, Window, "resources/shaders/compiled/vert.spv",
                  "resources/shaders/compiled/frag.spv"};
  if (!app.Init()) {
    SDL_Log("Couldn't init app.");
    return -1;
  }
  while (!app.ShouldQuit()) {
    if (!app.Poll()) {
      SDL_Log("App failed to Poll");
      break;
    }
    app.UpdateTime();
    if (app.ShouldQuit()) {
      break;
    }
    if (!app.Draw()) {
      SDL_Log("App failed to draw");
      break;
    };
  }

  SDL_Log("Releasing resources..");
  SDL_ReleaseWindowFromGPUDevice(Device, Window);
  SDL_DestroyWindow(Window);
  SDL_DestroyGPUDevice(Device);

  SDL_Log("Cleanup done");
  return 0;
}
