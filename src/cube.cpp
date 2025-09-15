#include "cube.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_sdlgpu3.h>
#include <imgui/imgui.h>

#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include "src/camera.h"
#include "util.h"

CubeProgram::CubeProgram(SDL_GPUDevice* device,
                         SDL_Window* window,
                         const char* vertex_path,
                         const char* fragment_path,
                         int w,
                         int h)
  : Program{ device, window }
  , vertex_path_{ vertex_path }
  , fragment_path_{ fragment_path }
  , vp_width_{ w }
  , vp_height_{ h }
{
  {
    scene_color_target_info_.clear_color = { 0.1f, 0.1f, 0.1f, 1.0f };
    scene_color_target_info_.load_op = SDL_GPU_LOADOP_CLEAR;
    scene_color_target_info_.store_op = SDL_GPU_STOREOP_STORE;
  }

  {
    scene_depth_target_info_.cycle = true;
    scene_depth_target_info_.clear_depth = 1;
    scene_depth_target_info_.clear_stencil = 0;
    scene_depth_target_info_.load_op = SDL_GPU_LOADOP_CLEAR;
    scene_depth_target_info_.store_op = SDL_GPU_STOREOP_STORE;
    scene_depth_target_info_.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
    scene_depth_target_info_.stencil_store_op = SDL_GPU_STOREOP_STORE;
  }

  {
    swapchain_target_info_.clear_color = { .1f, .1f, .1f, .1f };
    swapchain_target_info_.load_op = SDL_GPU_LOADOP_CLEAR;
    swapchain_target_info_.store_op = SDL_GPU_STOREOP_STORE;
    swapchain_target_info_.mip_level = 0;
    swapchain_target_info_.layer_or_depth_plane = 0;
    swapchain_target_info_.cycle = false;
  }

  {
    rotations_[0] = Rotation{
      "X Axis",
      &cube_transform_.rotation_.x,
      0.f,
    };
    rotations_[1] = Rotation{
      "Y Axis",
      &cube_transform_.rotation_.y,
      1.f,
    };
    rotations_[2] = Rotation{
      "Z Axis",
      &cube_transform_.rotation_.z,
      0.f,
    };
  }
}

CubeProgram::~CubeProgram()
{
  SDL_Log("Destroying app");
  SDL_ReleaseGPUShader(Device, vertex_);
  SDL_ReleaseGPUShader(Device, fragment_);
  SDL_ReleaseGPUGraphicsPipeline(Device, scene_pipeline_);
  SDL_ReleaseGPUGraphicsPipeline(Device, scene_wireframe_pipeline_);
  SDL_ReleaseGPUTexture(Device, depth_target_);
  SDL_ReleaseGPUTexture(Device, color_target_);
  SDL_ReleaseGPUBuffer(Device, vbuffer_);
  SDL_ReleaseGPUBuffer(Device, ibuffer_);
  SDL_Log("Released GPU Resources");

  SDL_WaitForGPUIdle(Device);
  ImGui_ImplSDL3_Shutdown();
  ImGui_ImplSDLGPU3_Shutdown();
  ImGui::DestroyContext();
}

bool
CubeProgram::Init()
{
  if (!InitGui()) {
    SDL_Log("Couldn't init imgui");
    return false;
  }
  SDL_Log("Started ImGui");
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

  SDL_GPUColorTargetDescription color_descs[] = { {
    .format = SDL_GetGPUSwapchainTextureFormat(Device, Window),
  } };
  SDL_GPUVertexAttribute vertex_attributes[] = {
    { .location = 0,
      .buffer_slot = 0,
      .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
      .offset = 0 },
    { .location = 1,
      .buffer_slot = 0,
      .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
      .offset = sizeof(float) * 3 }
  };

  SDL_GPUVertexBufferDescription vertex_desc[] = { {
    .slot = 0,
    .pitch = sizeof(PosUvVertex),
    .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    .instance_step_rate = 0,
  } };

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
              .cull_mode = SDL_GPU_CULLMODE_NONE,
              .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
          },
      .depth_stencil_state =
          {
              .compare_op = SDL_GPU_COMPAREOP_LESS,
              .write_mask = 0xFF,
              .enable_depth_test = true,
              .enable_depth_write = true,
              .enable_stencil_test = false,
          },
      .target_info =
          {
              .color_target_descriptions = color_descs,
              .num_color_targets = 1,
              .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
              .has_depth_stencil_target = true,
          },
  };

  scene_pipeline_ = SDL_CreateGPUGraphicsPipeline(Device, &pipelineCreateInfo);
  if (scene_pipeline_ == NULL) {
    SDL_Log("Couldn't create pipeline!");
    return false;
  }
  pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
  scene_wireframe_pipeline_ =
    SDL_CreateGPUGraphicsPipeline(Device, &pipelineCreateInfo);
  if (scene_wireframe_pipeline_ == NULL) {
    SDL_Log("Couldn't create wireframe pipeline!");
    return false;
  }
  SDL_Log("Created pipelines");

  if (!SendVertexData()) {
    SDL_Log("Couldn't send vertex data!");
    return false;
  }
  SDL_Log("Sent vertex data to GPU");

  if (!LoadTextures()) {
    SDL_Log("Couldn't load textures!");
    return false;
  }
  SDL_Log("Loaded textures");

  if (!CreateSceneRenderTargets()) {
    SDL_Log("Couldn't create render target textures!");
    return false;
  }
  SDL_Log("Created render target textures");

  cube_transform_.translation_ = { 0.f, 0.f, 0.0f };
  cube_transform_.scale_ = { 1.f, 1.f, 1.f };

  camera_.Position = glm::vec3{ 0.f, 1.f, -4.f };
  camera_.Target = glm::vec3{ 0.f, 0.f, 0.f };

  return true;
}

bool
CubeProgram::ShouldQuit()
{
  return quit;
}

bool
CubeProgram::Poll()
{
  SDL_Event evt;

  while (SDL_PollEvent(&evt)) {
    ImGui_ImplSDL3_ProcessEvent(&evt);
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

void
CubeProgram::UpdateScene()
{
  static float c = -1.f;

  for (const auto& rot : rotations_) {
    if (rot.speed != 0.f) {
      *rot.axis =
        glm::mod(*rot.axis + DeltaTime * rot.speed, glm::two_pi<float>());
    }
  }

  if (camera_.Position.z < -10.f) {
    c = 1.f;
  } else if (camera_.Position.z > -1.f) {
    c = -1.f;
  }
  // camera_.Position.z += c * DeltaTime * 2.5f;
  // camera_.Touched = true;
  cube_transform_.Touched = true;
  camera_.Update();
}

bool
CubeProgram::Draw()
{
  static const SDL_GPUViewport scene_vp{
    0, 0, float(vp_width_), float(vp_height_), 0.1f, 1.0f
  };
  static const SDL_GPUBufferBinding vBinding{ vbuffer_, 0 };
  static const SDL_GPUBufferBinding iBinding{ ibuffer_, 0 };

  SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(Device);
  if (cmdbuf == NULL) {
    SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
    return false;
  }

  SDL_GPUTexture* swapchainTexture;
  if (!SDL_WaitAndAcquireGPUSwapchainTexture(
        cmdbuf, Window, &swapchainTexture, NULL, NULL)) {
    SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
    return false;
  }
  if (swapchainTexture == NULL) {
    SDL_SubmitGPUCommandBuffer(cmdbuf);
    return true;
  }

  UpdateScene(); // TODO: move out
  static SDL_GPUTextureSamplerBinding sampler_bind{ cube_tex_, cube_sampler_ };
  MatricesBinding mvp{ camera_.Projection() * camera_.View(),
                       cube_transform_.Matrix() };
  auto draw_data = DrawGui();
  auto d = instance_cfg.dimension;
  auto total_instances = d * d * d;

  ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmdbuf);
  SDL_PushGPUVertexUniformData(cmdbuf, 0, &mvp, sizeof(mvp));
  SDL_PushGPUVertexUniformData(cmdbuf, 1, &instance_cfg, sizeof(instance_cfg));

  // Scene Pass
  {
    scene_color_target_info_.texture = color_target_;
    scene_depth_target_info_.texture = depth_target_;
    SDL_GPURenderPass* scenePass = SDL_BeginGPURenderPass(
      cmdbuf, &scene_color_target_info_, 1, &scene_depth_target_info_);

    SDL_BindGPUGraphicsPipeline(
      scenePass, wireframe_ ? scene_wireframe_pipeline_ : scene_pipeline_);
    SDL_BindGPUVertexBuffers(scenePass, 0, &vBinding, 1);
    SDL_BindGPUIndexBuffer(
      scenePass, &iBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_BindGPUFragmentSamplers(scenePass, 0, &sampler_bind, 1);
    SDL_SetGPUViewport(scenePass, &scene_vp);
    SDL_DrawGPUIndexedPrimitives(
      scenePass, INDEX_COUNT, total_instances, 0, 0, 0);
    SDL_EndGPURenderPass(scenePass);
  }

  // GUI Pass
  {
    swapchain_target_info_.texture = swapchainTexture;
    SDL_GPURenderPass* guiPass =
      SDL_BeginGPURenderPass(cmdbuf, &swapchain_target_info_, 1, nullptr);

    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmdbuf, guiPass);
    SDL_EndGPURenderPass(guiPass);
  }

  SDL_SubmitGPUCommandBuffer(cmdbuf);
  return true;
}

bool
CubeProgram::LoadShaders()
{
  vertex_ = LoadShader(vertex_path_, Device, 0, 2, 0, 0);
  if (vertex_ == nullptr) {
    SDL_Log("Couldn't load vertex shader at path %s", vertex_path_);
    return false;
  }
  fragment_ = LoadShader(fragment_path_, Device, 1, 1, 0, 0);
  if (fragment_ == nullptr) {
    SDL_Log("Couldn't load fragment shader at path %s", fragment_path_);
    return false;
  }
  return true;
}

bool
CubeProgram::LoadTextures()
{
  auto img = LoadImage("resources/textures/grass.png");
  if (!img) {
    SDL_Log("Couldn't load image: %s", SDL_GetError());
    return false;
  }
  SDL_GPUSamplerCreateInfo sampler_info{
    .min_filter = SDL_GPU_FILTER_NEAREST,
    .mag_filter = SDL_GPU_FILTER_NEAREST,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
  };
  cube_sampler_ = SDL_CreateGPUSampler(Device, &sampler_info);

  SDL_GPUTextureCreateInfo tex_info{ .type = SDL_GPU_TEXTURETYPE_2D,
                                     .format =
                                       SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                     .width = static_cast<Uint32>(img->w),
                                     .height = static_cast<Uint32>(img->h),
                                     .layer_count_or_depth = 1,
                                     .num_levels = 1,
                                     .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER };

  cube_tex_ = SDL_CreateGPUTexture(Device, &tex_info);

  SDL_GPUTransferBufferCreateInfo transfer_info{
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = (Uint32)img->w * (Uint32)img->h * 4
  };
  SDL_GPUTransferBuffer* textureTransferBuffer =
    SDL_CreateGPUTransferBuffer(Device, &transfer_info);

  void* textureTransferPtr =
    SDL_MapGPUTransferBuffer(Device, textureTransferBuffer, false);
  SDL_memcpy(textureTransferPtr, img->pixels, img->w * img->h * 4);
  SDL_UnmapGPUTransferBuffer(Device, textureTransferBuffer);

  SDL_GPUTextureTransferInfo tex_transfer_info{
    .transfer_buffer = textureTransferBuffer,
    .offset = 0,
  };
  SDL_GPUTextureRegion tex_reg{
    .texture = cube_tex_, .w = (Uint32)img->w, .h = (Uint32)img->h, .d = 1
  };

  {
    SDL_GPUCommandBuffer* cmdBuf = SDL_AcquireGPUCommandBuffer(Device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuf);
    SDL_UploadToGPUTexture(copyPass, &tex_transfer_info, &tex_reg, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmdBuf);
    SDL_DestroySurface(img);
    SDL_ReleaseGPUTransferBuffer(Device, textureTransferBuffer);
  }

  return true;
}

bool
CubeProgram::SendVertexData()
{
  SDL_GPUBufferCreateInfo vertInfo = { .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                       .size =
                                         sizeof(PosUvVertex) * VERT_COUNT };
  SDL_GPUBufferCreateInfo idxInfo = { .usage = SDL_GPU_BUFFERUSAGE_INDEX,
                                      .size = sizeof(Uint16) * INDEX_COUNT };
  SDL_GPUTransferBufferCreateInfo transferInfo = {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = (sizeof(PosUvVertex) * VERT_COUNT) + (sizeof(Uint16) * INDEX_COUNT)
  };

  vbuffer_ = SDL_CreateGPUBuffer(Device, &vertInfo);
  ibuffer_ = SDL_CreateGPUBuffer(Device, &idxInfo);
  SDL_GPUTransferBuffer* transferBuffer =
    SDL_CreateGPUTransferBuffer(Device, &transferInfo);

  if (!vbuffer_ || !ibuffer_ || !transferBuffer) {
    SDL_Log("couldn't create buffers");
    return false;
  }

  // Transfer Buffer to send vertex data to GPU
  PosUvVertex* transferData =
    (PosUvVertex*)SDL_MapGPUTransferBuffer(Device, transferBuffer, false);
  if (!transferData) {
    SDL_Log("couldn't get mapping for transfer buffer");
    return false;
  }

  for (Uint8 i = 0; i < VERT_COUNT; ++i) {
    transferData[i] = verts_uvs[i];
  }

  Uint16* indexData = (Uint16*)&transferData[VERT_COUNT];
  for (Uint8 i = 0; i < INDEX_COUNT; ++i) {
    indexData[i] = indices[i];
  }

  SDL_UnmapGPUTransferBuffer(Device, transferBuffer);

  // Upload the transfer data to the GPU resources
  SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(Device);
  if (!uploadCmdBuf) {
    SDL_Log("couldn't acquire command buffer");
    return false;
  }
  SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

  SDL_GPUTransferBufferLocation trLoc = { .transfer_buffer = transferBuffer,
                                          .offset = 0 };
  SDL_GPUBufferRegion reg = { .buffer = vbuffer_,
                              .offset = 0,
                              .size = sizeof(PosUvVertex) * VERT_COUNT };
  SDL_UploadToGPUBuffer(copyPass, &trLoc, &reg, false);

  trLoc.offset = sizeof(PosUvVertex) * VERT_COUNT;
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

bool
CubeProgram::CreateSceneRenderTargets()
{
  auto info =
    SDL_GPUTextureCreateInfo{ .type = SDL_GPU_TEXTURETYPE_2D,
                              .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                              .width = static_cast<Uint32>(vp_width_),
                              .height = static_cast<Uint32>(vp_height_),
                              .layer_count_or_depth = 1,
                              .num_levels = 1,
                              .sample_count = SDL_GPU_SAMPLECOUNT_1,
                              .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER |
                                       SDL_GPU_TEXTUREUSAGE_COLOR_TARGET };
  color_target_ = SDL_CreateGPUTexture(Device, &info);

  info.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
  info.usage =
    SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
  depth_target_ = SDL_CreateGPUTexture(Device, &info);

  return depth_target_ != nullptr && color_target_ != nullptr;
}

bool
CubeProgram::InitGui()
{
  float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
    ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
    ImGuiConfigFlags_NavEnableGamepad;              // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup scaling
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);
  style.FontScaleDpi = main_scale;

  // Setup Platform/Renderer backends
  if (!ImGui_ImplSDL3_InitForSDLGPU(Window)) {
    return false;
  };
  ImGui_ImplSDLGPU3_InitInfo init_info = {};
  init_info.Device = Device;
  init_info.ColorTargetFormat =
    SDL_GetGPUSwapchainTextureFormat(Device, Window);
  init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
  init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
  init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
  return ImGui_ImplSDLGPU3_Init(&init_info);
}

ImDrawData*
CubeProgram::DrawGui()
{
  // Init frame:
  {
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
  }

  // GUI stuff
  {
    if (ImGui::Begin("Scene")) {
      ImGui::Text("Hello world");
      ImGui::Image((ImTextureID)(intptr_t)color_target_,
                   ImVec2((float)vp_width_, (float)vp_height_));
      ImGui::End();
    }

    ImGui::ShowMetricsWindow();

    if (ImGui::Begin("Settings")) {
      if (ImGui::TreeNode("Camera")) {
        if (ImGui::SliderFloat("X", (float*)&camera_.Position.x, -50.f, 50.f) ||
            ImGui::SliderFloat("Y", (float*)&camera_.Position.y, -50.f, 50.f) ||
            ImGui::SliderFloat("Z", (float*)&camera_.Position.z, -50.f, 50.f)) {
          camera_.Touched = true;
        }
        ImGui::TreePop();
      }
      if (ImGui::TreeNode("Spin Cube")) {
        for (auto& rot : rotations_) {
          ImGui::InputFloat(rot.name, &rot.speed);
        }
        ImGui::TreePop();
      }
      if (ImGui::TreeNode("Instancing")) {
        ImGui::InputFloat("Spread", &instance_cfg.spread);
        ImGui::InputInt("Dimensions", (int*)&instance_cfg.dimension);
        ImGui::TreePop();
      }
      ImGui::Checkbox("Wireframe", &wireframe_);
      ImGui::End();
    }
  }

  ImGui::Render();
  return ImGui::GetDrawData();
}
