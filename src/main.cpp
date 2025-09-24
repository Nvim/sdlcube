#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_video.h>

#include "cube.h"
#include "src/logger.h"

int
main()
{
  Logger::Init();
  LOG_DEBUG("Init");
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
    return 1;
  }

  SDL_Log("Initialized SDL");

  auto Device =
    SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL |
                          SDL_GPU_SHADERFORMAT_MSL,
                        true,
                        NULL);

  if (Device == NULL) {
    SDL_Log("GPUCreateDevice failed");
    return -1;
  }
  SDL_Log("Created Device");

  auto Window = SDL_CreateWindow("Some cube idk", 1600, 1200, 0);
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

  { // app lifecycle
    CubeProgram app{ Device,
                     Window,
                     "resources/shaders/compiled/vert.spv",
                     "resources/shaders/compiled/frag.spv",
                     1200,
                     900 };

    if (!app.Init()) {
      SDL_Log("Couldn't init app.");
    } else {

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
    }
  }

  SDL_Log("Releasing resources..");
  SDL_ReleaseWindowFromGPUDevice(Device, Window);
  SDL_DestroyGPUDevice(Device);
  SDL_DestroyWindow(Window);

  SDL_Log("Cleanup done");
  return 0;
}
