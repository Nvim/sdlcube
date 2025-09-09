#include "cube.h"
#include "util.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

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
  SDL_GPUVertexAttribute vertex_attributes[] = {
      {.location = 0,
       .buffer_slot = 0,
       .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
       .offset = 0},
      {.location = 1,
       .buffer_slot = 0,
       .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
       .offset = sizeof(float) * 3}};

  SDL_GPUVertexBufferDescription vertex_desc[] = {{
      .slot = 0,
      .pitch = sizeof(Vertex),
      .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
      .instance_step_rate = 0,
  }};

  SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
      .vertex_shader = vertex_,
      .fragment_shader = fragment_,
      .vertex_input_state =
          {
              .vertex_buffer_descriptions = vertex_desc,
              .num_vertex_buffers = 1,
              .vertex_attributes = vertex_attributes,
              .num_vertex_attributes = 2,
          },
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

  if (!SendVertexData()) {
    SDL_Log("Couldn't send vertex data!");
    return false;
  }
  SDL_Log("Sent vertex data to GPU");

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
  static SDL_GPUViewport vp = {0, 0, 0, 0, 0.1f, 1.0f};
  static int w, h;
  static SDL_GPUBufferBinding vBinding = {.buffer = vbuffer_, .offset = 0};
  static SDL_GPUBufferBinding iBinding = {.buffer = ibuffer_, .offset = 0};
  static auto transform = glm::identity<glm::mat4>();
  transform =
      glm::rotate(transform, 1.0f * DeltaTime, glm::vec3{0.0f, 0.0f, 1.0f});

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
    colorTargetInfo.clear_color = {0.2f, 0.2f, 0.2f, 1.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    // SDL_PushGPUFragmentUniformData(cmdbuf, 0, &col, sizeof(col));
    SDL_PushGPUVertexUniformData(cmdbuf, 0, &transform, sizeof(transform));
    SDL_GPURenderPass *renderPass =
        SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, NULL);

    SDL_BindGPUGraphicsPipeline(renderPass, pipeline_);
    SDL_BindGPUVertexBuffers(renderPass, 0, &vBinding, 1);
    SDL_BindGPUIndexBuffer(renderPass, &iBinding,
                           SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_SetGPUViewport(renderPass, &vp);
    SDL_DrawGPUIndexedPrimitives(renderPass, INDEX_COUNT, 1, 0, 0, 0);
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
  fragment_ = LoadShader(fragment_path_, Device, 0, 1, 0, 0);
  if (fragment_ == nullptr) {
    SDL_Log("Couldn't load fragment shader at path %s", fragment_path_);
    return false;
  }
  return true;
}

bool CubeProgram::SendVertexData() {
  SDL_GPUBufferCreateInfo vertInfo = {.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                      .size = sizeof(Vertex) * VERT_COUNT};
  SDL_GPUBufferCreateInfo idxInfo = {.usage = SDL_GPU_BUFFERUSAGE_INDEX,
                                     .size = sizeof(Uint16) * INDEX_COUNT};
  SDL_GPUTransferBufferCreateInfo transferInfo = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = (sizeof(Vertex) * VERT_COUNT) + (sizeof(Uint16) * INDEX_COUNT)};

  vbuffer_ = SDL_CreateGPUBuffer(Device, &vertInfo);
  ibuffer_ = SDL_CreateGPUBuffer(Device, &idxInfo);
  SDL_GPUTransferBuffer *transferBuffer =
      SDL_CreateGPUTransferBuffer(Device, &transferInfo);

  if (!vbuffer_ || !ibuffer_ || !transferBuffer) {
    SDL_Log("couldn't create buffers");
    return false;
  }

  // Transfer Buffer to send vertex data to GPU
  Vertex *transferData =
      (Vertex *)SDL_MapGPUTransferBuffer(Device, transferBuffer, false);
  if (!transferData) {
    SDL_Log("couldn't get mapping for transfer buffer");
    return false;
  }

  for (Uint8 i = 0; i < VERT_COUNT; ++i) {
    transferData[i] = verts[i];
  }

  Uint16 *indexData = (Uint16 *)&transferData[VERT_COUNT];
  for (Uint8 i = 0; i < INDEX_COUNT; ++i) {
    indexData[i] = indices[i];
  }

  SDL_UnmapGPUTransferBuffer(Device, transferBuffer);

  // Upload the transfer data to the GPU resources
  SDL_GPUCommandBuffer *uploadCmdBuf = SDL_AcquireGPUCommandBuffer(Device);
  if (!uploadCmdBuf) {
    SDL_Log("couldn't acquire command buffer");
    return false;
  }
  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

  SDL_GPUTransferBufferLocation trLoc = {.transfer_buffer = transferBuffer,
                                         .offset = 0};
  SDL_GPUBufferRegion reg = {
      .buffer = vbuffer_, .offset = 0, .size = sizeof(Vertex) * VERT_COUNT};
  SDL_UploadToGPUBuffer(copyPass, &trLoc, &reg, false);

  trLoc.offset = sizeof(Vertex) * VERT_COUNT;
  reg.buffer = ibuffer_;
  reg.size = sizeof(Uint16) * INDEX_COUNT;

  SDL_UploadToGPUBuffer(copyPass, &trLoc, &reg, false);

  SDL_EndGPUCopyPass(copyPass);
  if (!SDL_SubmitGPUCommandBuffer(uploadCmdBuf)) {
    SDL_Log("couldn't submit copy pass command buffer");
    return false;
  }
  SDL_ReleaseGPUTransferBuffer(Device, transferBuffer);

  return true;
}
