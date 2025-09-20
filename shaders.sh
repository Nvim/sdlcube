#!/usr/bin/env bash

glslang resources/shaders/src/frag.frag -V -o resources/shaders/compiled/frag.spv;
glslang resources/shaders/src/vert.vert -V -o resources/shaders/compiled/vert.spv;

glslang resources/shaders/src/skybox.frag -V -o resources/shaders/compiled/skybox.frag.spv;
glslang resources/shaders/src/skybox.vert -V -o resources/shaders/compiled/skybox.vert.spv;
