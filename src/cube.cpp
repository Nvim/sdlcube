#include "cube.h"
#include "util.h"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL.h>

CubeProgram::CubeProgram(SDL_GPUDevice *device, SDL_Window *window,
                         const char *vertex_path, const char *fragment_path)
    : Program{device, window}, vertex_path_{vertex_path},
      fragment_path_{fragment_path} {}

CubeProgram::~CubeProgram() {
  if (vertex_) {
    SDL_ReleaseGPUShader(Device, vertex_);
    SDL_Log("Released vertex shader");
  }
  if (fragment_) {
    SDL_ReleaseGPUShader(Device, fragment_);
    SDL_Log("Released fragment shader");
  }
  if (pipeline_) {
    SDL_ReleaseGPUGraphicsPipeline(Device, pipeline_);
    SDL_Log("Released pipeline");
  }
}

bool CubeProgram::Init() {
  SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(Device);
  if (!(backendFormats & SDL_GPU_SHADERFORMAT_SPIRV)) {
    SDL_Log("Backend doesn't support SPRIR-V");
    return false;
  }

  if (!LoadShaders()) {
    SDL_Log("Couldn't load shaders");
    return false;
  }
  SDL_Log("Loaded shaders");

  SDL_GPUColorTargetDescription color_descs[] = {{
      .format = SDL_GetGPUSwapchainTextureFormat(Device, Window),
  }};
  SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
      .vertex_shader = vertex_,
      .fragment_shader = fragment_,
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .rasterizer_state =
          {
              .fill_mode = SDL_GPU_FILLMODE_FILL,
          },
      .target_info =
          {
              .color_target_descriptions = color_descs,
              .num_color_targets = 1,
          },
  };

  pipeline_ = SDL_CreateGPUGraphicsPipeline(Device, &pipelineCreateInfo);
  if (pipeline_ == NULL) {
    SDL_Log("Couldn't create pipeline!");
    return false;
  }
  SDL_Log("Created pipeline");

  return true;
}

bool CubeProgram::ShouldQuit() { return quit; }

bool CubeProgram::Poll() {
  SDL_Event evt;

  while (SDL_PollEvent(&evt)) {
    if (evt.type == SDL_EVENT_QUIT) {
      quit = true;
    } else if (evt.type == SDL_EVENT_KEY_DOWN) {
      if (evt.key.key == SDLK_ESCAPE) {
        quit = true;
      }
    }
  }
  return true;
}

bool CubeProgram::Draw() {
  static SDL_GPUViewport vp = { 0, 0, 0, 0, 0.1f, 1.0f };
  static int w, h;
  //
  SDL_GetWindowSizeInPixels(Window, &w, &h);
  vp.w = w;
  vp.h = h;

  SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(Device);
  if (cmdbuf == NULL) {
    SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
    return false;
  }

  SDL_GPUTexture *swapchainTexture;
  if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, Window, &swapchainTexture,
                                             NULL, NULL)) {
    SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
    return false;
  }
  if (swapchainTexture != NULL) {
    SDL_GPUColorTargetInfo colorTargetInfo{};
    colorTargetInfo.texture = swapchainTexture;
    colorTargetInfo.clear_color = {0.3f, 0.4f, 0.5f, 1.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass *renderPass =
        SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, NULL);

		SDL_BindGPUGraphicsPipeline(renderPass, pipeline_);
    SDL_SetGPUViewport(renderPass, &vp);
		SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(renderPass);
  }

  SDL_SubmitGPUCommandBuffer(cmdbuf);
  return true;
}

bool CubeProgram::LoadShaders() {
  vertex_ = LoadShader(vertex_path_, Device, 0, 0, 0, 0);
  if (vertex_ == nullptr) {
    SDL_Log("Couldn't load vertex shader at path %s", vertex_path_);
    return false;
  }
  fragment_ = LoadShader(fragment_path_, Device, 0, 0, 0, 0);
  if (fragment_ == nullptr) {
    SDL_Log("Couldn't load fragment shader at path %s", fragment_path_);
    return false;
  }
  return true;
}
