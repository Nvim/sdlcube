#pragma once

#include <SDL3/SDL_gpu.h>

struct PosVertex
{
  float pos[3];
};

struct PosColVertex
{
  float poscol[6];
};

struct PosUvVertex
{
  float pos[3];
  float uv[2];
};

SDL_GPUShader*
LoadShader(const char* path,
           SDL_GPUDevice* device,
           Uint32 samplerCount,
           Uint32 uniformBufferCount,
           Uint32 storageBufferCount,
           Uint32 storageTextureCount);

SDL_Surface*
LoadImage(const char* path);
