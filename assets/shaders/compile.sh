#!/bin/bash
glslangValidator shader.vert -V -o vert.spv
glslangValidator shader.frag -V -o frag.spv
glslangValidator compute.comp -V -o comp.spv
