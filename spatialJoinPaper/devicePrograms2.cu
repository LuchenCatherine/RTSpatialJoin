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
#include "LaunchParams2.h"
#include "devicePrograms2.cuh"

using namespace osc;

namespace osc {
  
  /*! launch parameters in constant memory, filled in by optix upon
      optixLaunch (this gets filled in from the buffer we pass to
      optixLaunch) */
  extern "C" __constant__ LaunchParams optixLaunchParams;

  // for this simple example, we have a single ray type
  enum { SURFACE_RAY_TYPE=0, RAY_TYPE_COUNT };

  struct AllHitsPRD
  {
    int raySource;     // which mesh the ray/edge belongs to, 0 for merged mesh (background mesh), 1 for query mesh
    int sourceID;       // which ray/edge is this/which mesh the ray/edge belongs to?
    int count;          // number of UNIQUE instances hit
    int overflow;
    unsigned int ids[256];

    // visited table: one flag per possible instance
    uint32_t visited[32];
  };
  
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
    // int &prd = *(int*)getPRD<int>();
    // prd = 1;

    AllHitsPRD* prd = getPRD<AllHitsPRD>();
    int id = optixGetInstanceId();

    if (prd->raySource == 1) 
    {
      // This is a hit from a ray from the query mesh, mark the corresponding mesh ID in the result buffer
      int mesh2Id = prd->sourceID;
      int mesh1Id = id;
      int index = mesh1Id * optixLaunchParams.numMeshes2 + mesh2Id;
      optixLaunchParams.resultBuffer[index] = true;
    }
    else
    {
      // This is a hit from a ray $s$ from the merged mesh, mark the corresponding mesh ID $s$ belongs to in the result buffer
      int mesh1Id = prd->sourceID;
      int mesh2Id = id;
      int index = mesh1Id * optixLaunchParams.numMeshes2 + mesh2Id;
      optixLaunchParams.resultBuffer[index] = true;
    }

    const unsigned int word = id >> 5;   // /32
    const unsigned int bit  = id & 31;   // %32

    if (word >= 32)
    {
        prd->overflow = 1;
        optixIgnoreIntersection();
        return;
    }

    const uint32_t mask = 1u << bit;

    if (prd->visited[word] & mask)
    {
        optixIgnoreIntersection();
        return;
    }

    prd->visited[word] |= mask;

    if (prd->count < 256)
        prd->ids[prd->count++] = id;
    else
        prd->overflow = 1;

    // printf("anyhit: instance id = %u, primitive id = %d\n", id, primID);
    optixIgnoreIntersection();

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
    // int &prd = *(int*)getPRD<int>();
    // prd = 0;
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

    auto ver1 = (vec3f*) optixLaunchParams.vertexBuffer1;
    auto ind1 = (vec3i*) optixLaunchParams.indexBuffer1;

    auto ver2 = (vec3f*) optixLaunchParams.vertexBuffer2;
    auto ind2 = (vec3i*) optixLaunchParams.indexBuffer2;

    auto result = (bool*) optixLaunchParams.resultBuffer;
    result[0] = false;

    int idx_p1, idx_p2;
    vec3f p1, p2, rayDir;

    // Total number of edges/ray segments
    int N_tests = 3 * (optixLaunchParams.numTriangles1 + optixLaunchParams.numTriangles2);
    int dimension_x = optixLaunchParams.dimension_x;


    // =========================================================
    // Initialize PRD HERE
    // =========================================================
    AllHitsPRD resultPRD = {};

    for (int thread_idx = ix; thread_idx < N_tests; thread_idx += dimension_x)
    {
      // our per-ray data for this example. what we initialize it to
      // won't matter, since this value will be overwritten by either
      // the miss or hit program, anyway

      // the values we store the PRD pointer in:
      uint32_t u0, u1;
      packPointer( &resultPRD, u0, u1 );


      // case 1: ray segments from merged mesh1 (background mesh) hit query mesh (mesh2)
      if (thread_idx/3 < optixLaunchParams.numTriangles1) {

        const int triangleId = thread_idx / 3;
        resultPRD.sourceID = optixLaunchParams.triangleToMeshId1[triangleId]; // store the triangle ID in the PRD
        resultPRD.raySource = 0; // this ray is from the merged mesh (mesh1), we will use this flag to determine which part of the result buffer to update in the anyhit program
        // directly compute the index in resultBuffer, 
        
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
        // case 2: test edges of query mesh (mesh2) hit merged mesh (mesh1)
        // resultPRD.sourceID = -1; // store the triangle ID in the PRD, -1 indicates this is an edge from the query mesh (mesh2)
        resultPRD.raySource = 1; // this ray is from the query mesh (mesh2), we will use this flag to determine which part of the result buffer to update in the anyhit program

        const int triangleId = thread_idx/ 3 - optixLaunchParams.numTriangles1;
        resultPRD.sourceID = optixLaunchParams.triangleToMeshId2[triangleId];
        
        vec3i triangle = ind2[thread_idx/ 3 - optixLaunchParams.numTriangles1];
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
      // result[thread_idx] = resultPRD;
      // atomicOr(result, resultPRD);

    }

    // printf("total hit count = %d, overflow = %d\n", resultPRD.count, resultPRD.overflow);
    // for (int i = 0; i < resultPRD.count; i++)
    //   printf("hit[%d] = %d\n", i, resultPRD.ids[i]);
  
  }
  
} // ::osc



// 05/20 Lu's thinking below:

//并不对称
// [i, j] = 1 means i query_mesh's triangle ID, j background_mesh's triangle ID
// segment from background mesh j intersected with query mesh with instance ID = i,
//  => mark the corresponding resultBuffer[i*len(background_mesh) + j] = 1
// segment from query mesh i intersected with background mesh with instance ID = j,
//  => mark the corresponding resultBuffer[len(background_mesh)*i + j] = 1
// we need to mark the source is query mesh or background mesh in the PRD, so that we know which part of the result buffer to update in the anyhit program

