// ======================================================================== //
// Copyright 2018-2019 Ingo Wald                                            //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "gdt/math/vec.h"
#include "optix7.h"
#include "CUDABuffer.h"

namespace osc {
  using namespace gdt;
  
  struct LaunchParams
  {
    // Same as LaunchParams.h
    // two traversable handles for two 3D triangle meshes for spatial queries, e.g., 3D intersection. 
    OptixTraversableHandle traversable1;
    OptixTraversableHandle traversable2;

    //  Buffers for the multiple meshes, e.g., vertex/index/offset buffers. Concatenate the buffers of multiple meshes into one buffer, and use offset to access the data of each mesh.
    vec3f* vertexBuffer1;
    vec3i* indexBuffer1;
    int* triangleToMeshId1; // for each triangle, store the original mesh ID it belongs to

    // sizes
    int numVertices1;
    int numTriangles1;
    int numMeshes1;

    // query mesh
    vec3f* vertexBuffer2;
    vec3i* indexBuffer2;
    int* triangleToMeshId2; // for each triangle, store the original mesh ID it belongs to

    //sizes
    int numVertices2;
    int numTriangles2;
    int numMeshes2;


    bool* resultBuffer;
    int resultBufferSize;

    // Launch dimension
    int dimension_x = 8;
  };

} // ::osc
