#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>

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

  bool quit = false;
  SDL_Event evt;
  while (!quit) {

    while (SDL_PollEvent(&evt)) {
      if (evt.type == SDL_EVENT_QUIT) {
        SDL_Log("Quitting");
        quit = true;
        break;
      } else if (evt.type == SDL_EVENT_KEY_DOWN) {
        if (evt.key.key == SDLK_ESCAPE) {
          SDL_Log("Quitting");
          quit = true;
          break;
        }
      }

      SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(Device);
      if (cmdbuf == NULL) {
        SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return -1;
      }

      SDL_GPUTexture *swapchainTexture;
      if (!SDL_WaitAndAcquireGPUSwapchainTexture(
              cmdbuf, Window, &swapchainTexture, NULL, NULL)) {
        SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return -1;
      }
      if (swapchainTexture != NULL) {
        SDL_GPUColorTargetInfo colorTargetInfo{};
        colorTargetInfo.texture = swapchainTexture;
        colorTargetInfo.clear_color = {0.3f, 0.4f, 0.5f, 1.0f};
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass *renderPass =
            SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, NULL);
        SDL_EndGPURenderPass(renderPass);
      }

      SDL_SubmitGPUCommandBuffer(cmdbuf);
    }
  }

  SDL_Log("Releasing resources..");
  SDL_ReleaseWindowFromGPUDevice(Device, Window);
  SDL_DestroyWindow(Window);
  SDL_DestroyGPUDevice(Device);

  SDL_Log("Cleanup done");
  return 0;
}
