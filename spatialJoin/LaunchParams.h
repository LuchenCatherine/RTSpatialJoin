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
    // two traversable handles for two 3D triangle meshes for spatial queries, e.g., 3D intersection. 
    OptixTraversableHandle traversable1;
    OptixTraversableHandle traversable2;

    CUDABuffer vertexBuffer1;
    CUDABuffer vertexBuffer2;
    CUDABuffer indexBuffer1;
    CUDABuffer indexBuffer2;

    CUDABuffer resultBuffer;

    // Launch dimension
    int dimension_x = 8;
  };

} // ::osc
