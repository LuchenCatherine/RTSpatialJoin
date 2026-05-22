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

#include <optix_device.h>

#include "LaunchParams.h"

using namespace osc;

namespace osc {
  
  /*! launch parameters in constant memory, filled in by optix upon
      optixLaunch (this gets filled in from the buffer we pass to
      optixLaunch) */
  extern "C" __constant__ LaunchParams optixLaunchParams;

  // for this simple example, we have a single ray type
  enum { SURFACE_RAY_TYPE=0, RAY_TYPE_COUNT };
  
  static __forceinline__ __device__
  void *unpackPointer( uint32_t i0, uint32_t i1 )
  {
    const uint64_t uptr = static_cast<uint64_t>( i0 ) << 32 | i1;
    void*           ptr = reinterpret_cast<void*>( uptr ); 
    return ptr;
  }

  static __forceinline__ __device__
  void  packPointer( void* ptr, uint32_t& i0, uint32_t& i1 )
  {
    const uint64_t uptr = reinterpret_cast<uint64_t>( ptr );
    i0 = uptr >> 32;
    i1 = uptr & 0x00000000ffffffff;
  }

  template<typename T>
  static __forceinline__ __device__ T *getPRD()
  { 
    const uint32_t u0 = optixGetPayload_0();
    const uint32_t u1 = optixGetPayload_1();
    return reinterpret_cast<T*>( unpackPointer( u0, u1 ) );
  }
  
  //------------------------------------------------------------------------------
  // closest hit and anyhit programs for radiance-type rays.
  //
  // Note eventually we will have to create one pair of those for each
  // ray type and each geometry type we want to render; but this
  // simple example doesn't use any actual geometries yet, so we only
  // create a single, dummy, set of them (we do have to have at least
  // one group of them to set up the SBT)
  //------------------------------------------------------------------------------
  
  extern "C" __global__ void __closesthit__radiance()
  {
    /*! for 3d spatial join, this will remain empty */  
  }
  
  extern "C" __global__ void __anyhit__radiance()
  {
    const int   primID = optixGetPrimitiveIndex();
    bool &prd = *(bool*)getPRD<bool>();
    prd = true;
  }


  
  //------------------------------------------------------------------------------
  // miss program that gets called for any ray that did not have a
  // valid intersection
  //
  // as with the anyhit/closest hit programs, in this example we only
  // need to have _some_ dummy function to set up a valid SBT
  // ------------------------------------------------------------------------------
  
  extern "C" __global__ void __miss__radiance()
  {
    bool &prd = *(bool*)getPRD<bool>();
    prd = false;
  }

  //------------------------------------------------------------------------------
  // ray gen program - the actual rendering happens in here
  //------------------------------------------------------------------------------
  extern "C" __global__ void __raygen__renderFrame()
  {
    // compute a test pattern based on pixel ID
    const int ix = optixGetLaunchIndex().x;
    const int iy = optixGetLaunchIndex().y;

    // return;

    auto vertexBuffer1 = optixLaunchParams.vertexBuffer1;
    auto indexBuffer1 = optixLaunchParams.indexBuffer1;
    auto vertexBuffer2 = optixLaunchParams.vertexBuffer2;
    auto indexBuffer2 = optixLaunchParams.indexBuffer2;

    CUdeviceptr d_vertices_1 = vertexBuffer1.d_pointer();
    CUdeviceptr d_indices_1  = indexBuffer1.d_pointer();
    CUdeviceptr d_vertices_2 = vertexBuffer2.d_pointer();
    CUdeviceptr d_indices_2  = indexBuffer2.d_pointer();

    auto ver1 = (vec3f*) d_vertices_1;
    auto ind1 = (vec3i*) d_indices_1;
    auto ver2 = (vec3f*) d_vertices_2;
    auto ind2 = (vec3i*) d_indices_2;

    auto result = (bool*) optixLaunchParams.resultBuffer.d_pointer();

    int idx_p1, idx_p2;
    vec3f p1, p2, rayDir;

    // Total number of edges/ray segments
    int N_tests = 3 * (indexBuffer1.size + indexBuffer2.size);
    int dimension_x = optixLaunchParams.dimension_x;

    result[ix] = true;

    for (int thread_idx = ix; thread_idx < N_tests; thread_idx += dimension_x)
    {
      // our per-ray data for this example. what we initialize it to
      // won't matter, since this value will be overwritten by either
      // the miss or hit program, anyway
      bool resultPRD = false;

      // the values we store the PRD pointer in:
      uint32_t u0, u1;
      packPointer( &resultPRD, u0, u1 );


      // case 1: test edges of mesh1 hit mesh2
      if (thread_idx/3 < indexBuffer1.size) {

        vec3i triangle = ind1[thread_idx/ 3];

        idx_p1 = triangle[thread_idx % 3];
        idx_p2 = triangle[(thread_idx + 1) % 3];

        // p1 is the origin
        p1 = ver1[idx_p1];
        p2 = ver1[idx_p2];

        // normalize ray direction
        rayDir = normalize(p2 - p1);

        // eucliean distance, common/gdt/gdt/math/vec.h
        float dist = length(p2 - p1);

        optixTrace(optixLaunchParams.traversable2,
               p1,
               rayDir,
               0.f,    // tmin
               dist,  // tmax
               0.0f,   // rayTime
               OptixVisibilityMask( 255 ),
               OPTIX_RAY_FLAG_DISABLE_CLOSESTHIT,//OPTIX_RAY_FLAG_NONE,
               SURFACE_RAY_TYPE,             // SBT offset
               RAY_TYPE_COUNT,               // SBT stride
               SURFACE_RAY_TYPE,             // missSBTIndex 
               u0, u1 );

      }
      else {
        // case 2: test edges of mesh2 hit mesh1

        vec3i triangle = ind2[thread_idx/ 3 - indexBuffer1.size];
        idx_p1 = triangle[thread_idx % 3];
        idx_p2 = triangle[(thread_idx + 1) % 3];

        // p1 is the origin
        p1 = ver2[idx_p1];
        p2 = ver2[idx_p2];

        // normalize ray direction
        rayDir = normalize(p2 - p1);

        // eucliean distance, common/gdt/gdt/math/vec.h
        float dist = length(p2 - p1);

        optixTrace(optixLaunchParams.traversable1,
                p1,
                rayDir,
                0.f,    // tmin
                dist,  // tmax
                0.0f,   // rayTime
                OptixVisibilityMask( 255 ),
                OPTIX_RAY_FLAG_DISABLE_CLOSESTHIT,//OPTIX_RAY_FLAG_NONE,
                SURFACE_RAY_TYPE,             // SBT offset
                RAY_TYPE_COUNT,               // SBT stride
                SURFACE_RAY_TYPE,             // missSBTIndex 
                u0, u1 );

      }

      // and write to frame buffer ...
      result[thread_idx] = resultPRD;

    }
  
  }
  
} // ::osc
