#pragma once

#include <SDL3/SDL_gpu.h>

SDL_GPUShader *LoadShader(const char *path, SDL_GPUDevice *device,
                          Uint32 samplerCount, Uint32 uniformBufferCount,
                          Uint32 storageBufferCount,
                          Uint32 storageTextureCount);
