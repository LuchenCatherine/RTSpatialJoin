#pragma once

// our own classes, partly shared between host and device
#include "CUDABuffer.h"
#include "LaunchParams2.h"
#include <fstream>
#include "gdt/math/AffineSpace.h"
#include <chrono>
#include <iostream>
#include <fstream>

/*! \namespace osc - Optix Siggraph Course */
namespace osc {

    struct TriangleMesh {
    
        void addCube(const vec3f &center, const vec3f &size);
        void addUnitCube(const affine3f &xfm);

        // add traingle mesh from path
        void addTriangleMesh(const std::string& off_file_path);
    
        std::vector<vec3f> vertex;
        std::vector<vec3i> index;
    };

    class SpatialJoinRenderer
    {
        // public accessible interface
        public: 
            /*! constructor - performs all setup, including initializing 
            optix, creates module, pipeline, programs, SBT, etc. */
            SpatialJoinRenderer();

            /*! get the spatial join result. */
            void render();
            
            void render(const std::vector<TriangleMesh>& models, const int launchDim);

            void query(const TriangleMesh &query_model);
            
            int query_single(const TriangleMesh &query_model, int idx);

            /*! download the rendered result */
            void downloadResult(int* h_result);


        protected:
            // internal helper functions
            /*! helper function that initializes optix and checks for errors */
            void initOptix();

            /*! creates and configures a optix device context (in this simple
            example, only for the primary GPU device) */
            void createContext();

            /*! creates the module that contains all the programs we are going
            to use. in this simple example, we use a single module from a
            single .cu file, using a single embedded ptx string */
            void createModule();

            /*! does all setup for the raygen program(s) we are going to use */
            void createRaygenPrograms();

            /*! does all setup for the miss program(s) we are going to use */
            void createMissPrograms();

            /*! does all setup for the hitgroup program(s) we are going to use */
            void createHitgroupPrograms();

            /*! assembles the full pipeline of all programs */
            void createPipeline();

            /*! constructs the shader binding table */
            void buildSBT();

            /*! upload mesh data vector*/
            void uploadMeshData(const std::vector<TriangleMesh>& models);
            
            /*! build an acceleration structure for the given triangle mesh */
            OptixTraversableHandle buildAccel(const TriangleMesh &model, CUDABuffer &vertexBuffer, CUDABuffer &indexBuffer, CUDABuffer &asBuffer);

            /*! build an array of traversables for the given array of models */
            void buildAccel(const std::vector<TriangleMesh>& models, std::vector<CUDABuffer>& vertexBuffers, 
                std::vector<CUDABuffer>& indexBuffers, std::vector<CUDABuffer>& asBuffers, std::vector<OptixTraversableHandle>& traversables);
            

        protected: 
            /*! @{ CUDA device context and stream that optix pipeline will run
            on, as well as device properties for this device */
            CUcontext          cudaContext;
            CUstream           stream;
            cudaDeviceProp     deviceProps;

            //! the optix context that our pipeline will run in.
            OptixDeviceContext optixContext;

            /*! @{ the pipeline we're building */
            OptixPipeline               pipeline;
            OptixPipelineCompileOptions pipelineCompileOptions = {};
            OptixPipelineLinkOptions    pipelineLinkOptions = {};

            /*! @{ the module that contains out device programs */
            OptixModule                 module;
            OptixModuleCompileOptions   moduleCompileOptions = {};

            /*! vector of all our program(group)s, and the SBT built around
            them */
            std::vector<OptixProgramGroup> raygenPGs;
            CUDABuffer raygenRecordsBuffer;
            std::vector<OptixProgramGroup> missPGs;
            CUDABuffer missRecordsBuffer;
            std::vector<OptixProgramGroup> hitgroupPGs;
            CUDABuffer hitgroupRecordsBuffer;
            OptixShaderBindingTable sbt = {};

            /*! @{ our launch parameters, on the host, and the buffer to store
            them on the device */
            LaunchParams launchParams;
            CUDABuffer   launchParamsBuffer;

            /*! the model we are going to trace rays against */
            int triangle_face_sum = 0;
            int n_models = 0;

            std::vector<CUDABuffer> vertexBuffers;
            std::vector<CUDABuffer> indexBuffers;

            //! buffer that keeps the (final, compacted) accel structure
            std::vector<CUDABuffer> asBuffers;
            std::vector<OptixTraversableHandle> traversables;
            
            /*! For query model */
            CUDABuffer vertexBuffer2;
            CUDABuffer indexBuffer2;
            OptixTraversableHandle traversable2;

            CUDABuffer asBuffer2;

            /*! keep the result */
            CUDABuffer resultBuffer;

    };



}
