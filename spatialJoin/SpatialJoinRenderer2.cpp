#include "SpatialJoinRenderer2.h"
// this include may only appear in a single source file:
#include <optix_function_table_definition.h>

/*! \namespace osc - Optix Siggraph Course */
namespace osc {

    extern "C" char embedded_ptx_code[];

    /*! SBT record for a raygen program */
    struct __align__( OPTIX_SBT_RECORD_ALIGNMENT ) RaygenRecord
    {
        __align__( OPTIX_SBT_RECORD_ALIGNMENT ) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
        // just a dummy value - later examples will use more interesting
        // data here
        void *data;
    };

    /*! SBT record for a miss program */
    struct __align__( OPTIX_SBT_RECORD_ALIGNMENT ) MissRecord
    {
        __align__( OPTIX_SBT_RECORD_ALIGNMENT ) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
        // just a dummy value - later examples will use more interesting
        // data here
        void *data;
    };


    /*! SBT record for a hitgroup program */
    struct __align__( OPTIX_SBT_RECORD_ALIGNMENT ) HitgroupRecord
    {
        __align__( OPTIX_SBT_RECORD_ALIGNMENT ) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
        // just a dummy value - later examples will use more interesting
        // data here
        int objectID;
    };


      //! add aligned cube with front-lower-left corner and size
    void TriangleMesh::addCube(const vec3f &center, const vec3f &size)
    {
        affine3f xfm;
        xfm.p = center - 0.5f*size;
        xfm.l.vx = vec3f(size.x,0.f,0.f);
        xfm.l.vy = vec3f(0.f,size.y,0.f);
        xfm.l.vz = vec3f(0.f,0.f,size.z);
        addUnitCube(xfm);
    }

    /*! add a unit cube (subject to given xfm matrix) to the current
        triangleMesh */
    void TriangleMesh::addUnitCube(const affine3f &xfm)
    {
        int firstVertexID = (int)vertex.size();
        vertex.push_back(xfmPoint(xfm,vec3f(0.f,0.f,0.f)));
        vertex.push_back(xfmPoint(xfm,vec3f(1.f,0.f,0.f)));
        vertex.push_back(xfmPoint(xfm,vec3f(0.f,1.f,0.f)));
        vertex.push_back(xfmPoint(xfm,vec3f(1.f,1.f,0.f)));
        vertex.push_back(xfmPoint(xfm,vec3f(0.f,0.f,1.f)));
        vertex.push_back(xfmPoint(xfm,vec3f(1.f,0.f,1.f)));
        vertex.push_back(xfmPoint(xfm,vec3f(0.f,1.f,1.f)));
        vertex.push_back(xfmPoint(xfm,vec3f(1.f,1.f,1.f)));


        int indices[] = {0,1,3, 2,0,3,
                        5,7,6, 5,6,4,
                        0,4,5, 0,5,1,
                        2,3,7, 2,7,6,
                        1,5,7, 1,7,3,
                        4,0,2, 4,2,6
                        };
        for (int i=0;i<12;i++)
        index.push_back(firstVertexID+vec3i(indices[3*i+0],
                                            indices[3*i+1],
                                            indices[3*i+2]));
    }

    // OFF Loader
    void TriangleMesh::addTriangleMesh(const std::string& off_file_path)
    {
        std::cout << GDT_TERMINAL_YELLOW;
        // std::cout << "#spatial join: loading 3D object(.off) file: " << off_file_path << std::endl;
        std::cout << GDT_TERMINAL_DEFAULT;
        std::ifstream ifs(off_file_path, std::ifstream::in);

        std::string line;
        int numv_value = 0, numtri_value = 0;
        int linesReadCounter = 0;

        int cur_vertex = 0, cur_tri = 0;

        float x, y, z;
        int idx_x, idx_y, idx_z;
        int face_num_v;

        while (ifs.good() && !ifs.eof() && std::getline(ifs, line)){
        if (linesReadCounter == 0) {linesReadCounter ++; continue;}
        if (line == "") continue;
        
        std::stringstream stringstream(line);
        if (linesReadCounter == 1){
            stringstream >> numv_value >> numtri_value; 
            linesReadCounter ++; 
            continue;
        }

        if (cur_vertex < numv_value)
        {
            stringstream >> x >> y >> z;
            vertex.push_back(vec3f(x, y, z));
            cur_vertex++;
        }
        else if (cur_tri < numtri_value)
        {
            stringstream >> face_num_v;

            if (face_num_v != 3) {
                printf("face_num_v: %d,line: %d\n", face_num_v, linesReadCounter);
            throw std::runtime_error("Not a triangle mesh!");
            }
            stringstream >> idx_x >> idx_y >> idx_z;
            index.push_back(vec3i(idx_x, idx_y, idx_z));
            cur_tri++;
        }
        
        linesReadCounter++;

        }// File Line Stream Loop
        // printf("\tThe model in the file have : %d trinagles and %d vertices\n", numtri_value, numv_value);

        ifs.close();
    
    }

    /*! constructor - performs all setup, including initializing
    optix, creates module, pipeline, programs, SBT, etc. */

    SpatialJoinRenderer::SpatialJoinRenderer()
    {

        std::cout << "#spatial join: init, create context, create module ..." << std::endl;
        initOptix();
        createContext();
        createModule();

        std::cout << "#spatial join: create raygen, miss, hit programs ..." << std::endl;
        createRaygenPrograms();
        createMissPrograms();
        createHitgroupPrograms();

        std::cout << "#spatial join: create pipline ..." << std::endl;
        createPipeline();

        std::cout << "#spatial join: build SBT table ..." << std::endl;
        buildSBT();

    }


    /* Build spatial index for static rendered models*/
    void SpatialJoinRenderer::render(const std::vector<TriangleMesh>& models, const int launchDim)
    {

        std::cout << "#spatial join: upload model data ..." << std::endl;
        uploadMeshData(models);

        std::cout << "#spatial join: build acceleration structures ..." << std::endl;
        n_models = models.size();
        std::cout << "Total models: " << n_models << std::endl;

        buildAccel(models, vertexBuffers, indexBuffers, asBuffers, traversables);
        
        std::cout << "#spatial join: launchDim: " << launchDim << std::endl;
        launchParams.dimension_x = launchDim;

    }


    void SpatialJoinRenderer::query(const TriangleMesh &query_model) 
    {

        vertexBuffer2.alloc_and_upload(query_model.vertex);
        indexBuffer2.alloc_and_upload(query_model.index);
        std::cout << "Total models: " << n_models << std::endl;
        traversable2 = buildAccel(query_model, vertexBuffer2, indexBuffer2, asBuffer2);

        for (int i = 0; i < n_models; i++) query_single(query_model, i);
    }

    
    int SpatialJoinRenderer::query_single(const TriangleMesh &query_model, int idx) {

        resultBuffer.alloc(sizeof(int));
        
        launchParams.vertexBuffer1 = vertexBuffers[idx];
        launchParams.indexBuffer1 = indexBuffers[idx];
        launchParams.traversable1 = traversables[idx];

        launchParams.vertexBuffer2 = vertexBuffer2;
        launchParams.indexBuffer2 = indexBuffer2;
        launchParams.traversable2 = traversable2;
        
        launchParams.resultBuffer = resultBuffer;
        launchParamsBuffer.alloc(sizeof(launchParams));
        launchParamsBuffer.upload(&launchParams,1);

        OPTIX_CHECK(optixLaunch(/*! pipeline we're launching launch: */
            pipeline,stream,
            /*! parameters and SBT */
            launchParamsBuffer.d_pointer(),
            launchParamsBuffer.sizeInBytes,
            &sbt,
            /*! dimensions of the launch: */
            launchParams.dimension_x, // 2^16
            1,
            1
            ));
        // sync - make sure the frame is rendered before we download and
        // display (obviously, for a high-performance application you
        // want to use streams and double-buffering, but for this simple
        // example, this will have to do)
        CUDA_SYNC_CHECK();
        
        int result = 0;
        downloadResult(&result);

        return result;

    }

    /*! batch build acceleration structures*/
    void SpatialJoinRenderer::buildAccel(const std::vector<TriangleMesh>& models, std::vector<CUDABuffer>& vertexBuffers, 
        std::vector<CUDABuffer>& indexBuffers, std::vector<CUDABuffer>& asBuffers, std::vector<OptixTraversableHandle>& traversables) 
    {
        

        for (int i = 0; i < models.size(); i++) {
            std::cout << "model " << i << std::endl;
            traversables[i] = buildAccel(models[i], vertexBuffers[i], indexBuffers[i], asBuffers[i]);
        }

    }


    /*! batch upload for models*/
    void SpatialJoinRenderer::uploadMeshData(const std::vector<TriangleMesh>& models) {

        // the number of models
        int N = models.size();

        // free memory on device
        for (auto& buffer: vertexBuffers) buffer.free();
        for (auto& buffer: indexBuffers) buffer.free();
        for(auto& buffer: asBuffers) buffer.free();

        // free memory on host
        vertexBuffers.clear();
        indexBuffers.clear();
        asBuffers.clear();

        // re-allocate memory on host for new models
        vertexBuffers.resize(N);
        indexBuffers.resize(N);
        asBuffers.resize(N);
        traversables.resize(N);

        // upload models from host to device
        for (int i = 0; i < N; i++) {
            vertexBuffers[i].alloc_and_upload(models[i].vertex);
            indexBuffers[i].alloc_and_upload(models[i].index);
        }

    }

    OptixTraversableHandle SpatialJoinRenderer::buildAccel(const TriangleMesh &model, CUDABuffer &vertexBuffer, CUDABuffer &indexBuffer, CUDABuffer &asBuffer)
    {
        // upload the model to the device: the builder, move upload mesh data to uploadMeshData function
        // vertexBuffer.alloc_and_upload(model.vertex);
        // indexBuffer.alloc_and_upload(model.index);

        OptixTraversableHandle asHandle { 0 };

        // ==================================================================
        // triangle inputs
        // ==================================================================
        OptixBuildInput triangleInput = {};
        triangleInput.type
        = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;

        // create local variables, because we need a *pointer* to the
        // device pointers
        CUdeviceptr d_vertices = vertexBuffer.d_pointer();
        CUdeviceptr d_indices  = indexBuffer.d_pointer();

        triangleInput.triangleArray.vertexFormat        = OPTIX_VERTEX_FORMAT_FLOAT3;
        triangleInput.triangleArray.vertexStrideInBytes = sizeof(vec3f);
        triangleInput.triangleArray.numVertices         = (int)model.vertex.size();
        triangleInput.triangleArray.vertexBuffers       = &d_vertices;

        triangleInput.triangleArray.indexFormat         = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
        triangleInput.triangleArray.indexStrideInBytes  = sizeof(vec3i);
        triangleInput.triangleArray.numIndexTriplets    = (int)model.index.size();
        triangleInput.triangleArray.indexBuffer         = d_indices;

        uint32_t triangleInputFlags[1] = { 0 };

        // in this example we have one SBT entry, and no per-primitive
        // materials:
        triangleInput.triangleArray.flags               = triangleInputFlags;
        triangleInput.triangleArray.numSbtRecords               = 1;
        triangleInput.triangleArray.sbtIndexOffsetBuffer        = 0;
        triangleInput.triangleArray.sbtIndexOffsetSizeInBytes   = 0;
        triangleInput.triangleArray.sbtIndexOffsetStrideInBytes = 0;

        // ==================================================================
        // BLAS setup
        // ==================================================================

        OptixAccelBuildOptions accelOptions = {};
        accelOptions.buildFlags             = OPTIX_BUILD_FLAG_NONE | OPTIX_BUILD_FLAG_ALLOW_COMPACTION;
        accelOptions.motionOptions.numKeys  = 1;
        accelOptions.operation              = OPTIX_BUILD_OPERATION_BUILD;

        OptixAccelBufferSizes blasBufferSizes;
        OPTIX_CHECK(optixAccelComputeMemoryUsage
                (optixContext,
                 &accelOptions,
                 &triangleInput,
                 1,  // num_build_inputs
                 &blasBufferSizes
                 ));

        // ==================================================================
        // prepare compaction
        // ==================================================================
        

        CUDABuffer compactedSizeBuffer;
        compactedSizeBuffer.alloc(sizeof(uint64_t));

        OptixAccelEmitDesc emitDesc;
        emitDesc.type   = OPTIX_PROPERTY_TYPE_COMPACTED_SIZE;
        emitDesc.result = compactedSizeBuffer.d_pointer();

        // ==================================================================
        // execute build (main stage)
        // ==================================================================
        // 1. 
        cudaEvent_t startEvent, endEvent;
        cudaEventCreate(&startEvent);
        cudaEventCreate(&endEvent);
        // 2. 
        cudaError_t err = cudaEventRecord(startEvent, stream);

        CUDABuffer tempBuffer;
        tempBuffer.alloc(blasBufferSizes.tempSizeInBytes);

        CUDABuffer outputBuffer;
        outputBuffer.alloc(blasBufferSizes.outputSizeInBytes);
        
        if (err != cudaSuccess) {
            std::cerr << "Error recording start event: " << cudaGetErrorString(err) << std::endl;
        }
        
        // 4. Record the end time (after the OptiX operation)
        cudaEventRecord(endEvent, stream);

        // 5. Synchronize and calculate the elapsed time
        cudaEventSynchronize(endEvent);  // Ensure the events have finished
        float elapsedTime;
        cudaEventElapsedTime(&elapsedTime, startEvent, endEvent);  // Get the time in milliseconds

        // 6. Print the result
        std::cout << "OptiX operation took " << elapsedTime << " ms" << std::endl;

        // 7. Cleanup events
        cudaEventDestroy(startEvent);
        cudaEventDestroy(endEvent);

        OPTIX_CHECK(optixAccelBuild(optixContext,
                                /* stream */0,
                                &accelOptions,
                                &triangleInput,
                                1,
                                tempBuffer.d_pointer(),
                                tempBuffer.sizeInBytes,

                                outputBuffer.d_pointer(),
                                outputBuffer.sizeInBytes,

                                &asHandle,

                                &emitDesc,1
                                ));


        CUDA_SYNC_CHECK();

        

        // ==================================================================
        // perform compaction
        // ==================================================================
        uint64_t compactedSize;
        compactedSizeBuffer.download(&compactedSize,1);

        asBuffer.alloc(compactedSize);
        OPTIX_CHECK(optixAccelCompact(optixContext,
                                  /*stream:*/0,
                                  asHandle,
                                  asBuffer.d_pointer(),
                                  asBuffer.sizeInBytes,
                                  &asHandle));
        CUDA_SYNC_CHECK();

        // ==================================================================
        // aaaaaand .... clean up
        // ==================================================================
        outputBuffer.free(); // << the UNcompacted, temporary output buffer
        tempBuffer.free();
        compactedSizeBuffer.free();

        return asHandle;


    }


    /*! helper function that initializes optix and checks for errors */
    void SpatialJoinRenderer::initOptix()
    {
        std::cout << "#spatial join: initializing optix..." << std::endl;

        // -------------------------------------------------------
        // check for available optix7 capable devices
        // -------------------------------------------------------
        cudaFree(0);
        int numDevices;
        cudaGetDeviceCount(&numDevices);
        if (numDevices == 0)
            throw std::runtime_error("#spatial join: no CUDA capable devices found!");
        std::cout << "#spatial join: found " << numDevices << " CUDA devices" << std::endl;

        // -------------------------------------------------------
        // initialize optix
        // -------------------------------------------------------
        OPTIX_CHECK( optixInit() );
        std::cout << GDT_TERMINAL_GREEN
              << "#spatial join: successfully initialized optix... yay!"
              << GDT_TERMINAL_DEFAULT << std::endl;
    }


    static void context_log_cb(unsigned int level,
                            const char *tag,
                            const char *message,
                            void *)
    {
        fprintf( stderr, "[%2d][%12s]: %s\n", (int)level, tag, message );
    }


    /*! creates and configures a optix device context (in this simple
    example, only for the primary GPU device) */
    void SpatialJoinRenderer::createContext()
    {
        // for this sample, do everything on one device
        const int deviceID = 0;
        CUDA_CHECK(SetDevice(deviceID));
        CUDA_CHECK(StreamCreate(&stream));

        cudaGetDeviceProperties(&deviceProps, deviceID);
        std::cout << "#spatial join: running on device: " << deviceProps.name << std::endl;

        CUresult  cuRes = cuCtxGetCurrent(&cudaContext);
        if( cuRes != CUDA_SUCCESS )
            fprintf( stderr, "Error querying current context: error code %d\n", cuRes );

        OPTIX_CHECK(optixDeviceContextCreate(cudaContext, 0, &optixContext));
        OPTIX_CHECK(optixDeviceContextSetLogCallback(optixContext,context_log_cb,nullptr,4));
    }


    /*! creates the module that contains all the programs we are going
      to use. in this simple example, we use a single module from a
      single .cu file, using a single embedded ptx string */
    void SpatialJoinRenderer::createModule()
    {
        moduleCompileOptions.maxRegisterCount  = 50;
        moduleCompileOptions.optLevel          = OPTIX_COMPILE_OPTIMIZATION_DEFAULT;
        moduleCompileOptions.debugLevel        = OPTIX_COMPILE_DEBUG_LEVEL_NONE;

        pipelineCompileOptions = {};
        pipelineCompileOptions.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
        pipelineCompileOptions.usesMotionBlur     = false;
        pipelineCompileOptions.numPayloadValues   = 2;
        pipelineCompileOptions.numAttributeValues = 2;
        pipelineCompileOptions.exceptionFlags     = OPTIX_EXCEPTION_FLAG_NONE;
        pipelineCompileOptions.pipelineLaunchParamsVariableName = "optixLaunchParams";

        pipelineLinkOptions.maxTraceDepth          = 2;

        const std::string ptxCode = embedded_ptx_code;

        char log[2048];
        size_t sizeof_log = sizeof( log );
        #if OPTIX_VERSION >= 70700
            OPTIX_CHECK(optixModuleCreate(optixContext,
                                        &moduleCompileOptions,
                                        &pipelineCompileOptions,
                                        ptxCode.c_str(),
                                        ptxCode.size(),
                                        log,&sizeof_log,
                                        &module
                                        ));
        #else
            OPTIX_CHECK(optixModuleCreateFromPTX(optixContext,
                                        &moduleCompileOptions,
                                        &pipelineCompileOptions,
                                        ptxCode.c_str(),
                                        ptxCode.size(),
                                        log,      // Log string
                                        &sizeof_log,// Log string sizse
                                        &module
                                        ));
        #endif
        if (sizeof_log > 1) PRINT(log);
    }

    /*! does all setup for the raygen program(s) we are going to use */
    void SpatialJoinRenderer::createRaygenPrograms()
    {
        // we do a single ray gen program in this example:
        raygenPGs.resize(1);

        OptixProgramGroupOptions pgOptions = {};
        OptixProgramGroupDesc pgDesc    = {};
        pgDesc.kind                     = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
        pgDesc.raygen.module            = module;
        pgDesc.raygen.entryFunctionName = "__raygen__renderFrame";

        // OptixProgramGroup raypg;
        char log[2048];
        size_t sizeof_log = sizeof( log );
        OPTIX_CHECK(optixProgramGroupCreate(optixContext,
                                        &pgDesc,
                                        1,
                                        &pgOptions,
                                        log,&sizeof_log,
                                        &raygenPGs[0]
                                        ));
        if (sizeof_log > 1) PRINT(log);
    }


    /*! does all setup for the miss program(s) we are going to use */
    void SpatialJoinRenderer::createMissPrograms()
    {
        // we do a single ray gen program in this example:
        missPGs.resize(1);

        OptixProgramGroupOptions pgOptions = {};
        OptixProgramGroupDesc pgDesc    = {};
        pgDesc.kind                     = OPTIX_PROGRAM_GROUP_KIND_MISS;
        pgDesc.miss.module            = module;
        pgDesc.miss.entryFunctionName = "__miss__radiance";

        // OptixProgramGroup raypg;
        char log[2048];
        size_t sizeof_log = sizeof( log );
        OPTIX_CHECK(optixProgramGroupCreate(optixContext,
                                        &pgDesc,
                                        1,
                                        &pgOptions,
                                        log,&sizeof_log,
                                        &missPGs[0]
                                        ));
        if (sizeof_log > 1) PRINT(log);
    }
    
    /*! does all setup for the hitgroup program(s) we are going to use */
    void SpatialJoinRenderer::createHitgroupPrograms()
    {
        // for this simple example, we set up a single hit group
        hitgroupPGs.resize(1);

        OptixProgramGroupOptions pgOptions = {};
        OptixProgramGroupDesc pgDesc    = {};
        pgDesc.kind                     = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
        pgDesc.hitgroup.moduleCH            = module;
        pgDesc.hitgroup.entryFunctionNameCH = "__closesthit__radiance";
        pgDesc.hitgroup.moduleAH            = module;
        pgDesc.hitgroup.entryFunctionNameAH = "__anyhit__radiance";

        char log[2048];
        size_t sizeof_log = sizeof( log );
        OPTIX_CHECK(optixProgramGroupCreate(optixContext,
                                        &pgDesc,
                                        1,
                                        &pgOptions,
                                        log,&sizeof_log,
                                        &hitgroupPGs[0]
                                        ));
        if (sizeof_log > 1) PRINT(log);
    }


    /*! assembles the full pipeline of all programs */
    void SpatialJoinRenderer::createPipeline()
    {
        std::vector<OptixProgramGroup> programGroups;
        for (auto pg : raygenPGs)
            programGroups.push_back(pg);
        for (auto pg : missPGs)
            programGroups.push_back(pg);
        for (auto pg : hitgroupPGs)
            programGroups.push_back(pg);

        char log[2048];
        size_t sizeof_log = sizeof( log );
        OPTIX_CHECK(optixPipelineCreate(optixContext,
                                    &pipelineCompileOptions,
                                    &pipelineLinkOptions,
                                    programGroups.data(),
                                    (int)programGroups.size(),
                                    log,&sizeof_log,
                                    &pipeline
                                    ));
        if (sizeof_log > 1) PRINT(log);

        OPTIX_CHECK(optixPipelineSetStackSize
                (/* [in] The pipeline to configure the stack size for */
                 pipeline,
                 /* [in] The direct stack size requirement for direct
                    callables invoked from IS or AH. */
                 2*1024,
                 /* [in] The direct stack size requirement for direct
                    callables invoked from RG, MS, or CH.  */
                 2*1024,
                 /* [in] The continuation stack requirement. */
                 2*1024,
                 /* [in] The maximum depth of a traversable graph
                    passed to trace. */
                 1));
        if (sizeof_log > 1) PRINT(log);
    }

    /*! constructs the shader binding table */
    void SpatialJoinRenderer::buildSBT()
    {
        // ------------------------------------------------------------------
        // build raygen records
        // ------------------------------------------------------------------
        std::vector<RaygenRecord> raygenRecords;
        for (int i=0;i<raygenPGs.size();i++) {
            RaygenRecord rec;
            OPTIX_CHECK(optixSbtRecordPackHeader(raygenPGs[i],&rec));
            rec.data = nullptr; /* for now ... */
            raygenRecords.push_back(rec);
        }
        raygenRecordsBuffer.alloc_and_upload(raygenRecords);
        sbt.raygenRecord = raygenRecordsBuffer.d_pointer();

        // ------------------------------------------------------------------
        // build miss records
        // ------------------------------------------------------------------
        std::vector<MissRecord> missRecords;
        for (int i=0;i<missPGs.size();i++) {
            MissRecord rec;
            OPTIX_CHECK(optixSbtRecordPackHeader(missPGs[i],&rec));
            rec.data = nullptr; /* for now ... */
            missRecords.push_back(rec);
        }
        missRecordsBuffer.alloc_and_upload(missRecords);
        sbt.missRecordBase          = missRecordsBuffer.d_pointer();
        sbt.missRecordStrideInBytes = sizeof(MissRecord);
        sbt.missRecordCount         = (int)missRecords.size();

        // ------------------------------------------------------------------
        // build hitgroup records
        // ------------------------------------------------------------------

        // we don't actually have any objects in this example, but let's
        // create a dummy one so the SBT doesn't have any null pointers
        // (which the sanity checks in compilation would complain about)
        int numObjects = 1;
        std::vector<HitgroupRecord> hitgroupRecords;
        for (int i=0;i<numObjects;i++) {
            int objectType = 0;
            HitgroupRecord rec;
            OPTIX_CHECK(optixSbtRecordPackHeader(hitgroupPGs[objectType],&rec));
            rec.objectID = i;
            hitgroupRecords.push_back(rec);
        }
        hitgroupRecordsBuffer.alloc_and_upload(hitgroupRecords);
        sbt.hitgroupRecordBase          = hitgroupRecordsBuffer.d_pointer();
        sbt.hitgroupRecordStrideInBytes = sizeof(HitgroupRecord);
        sbt.hitgroupRecordCount         = (int)hitgroupRecords.size();
    }


    /*! render one frame */
    void SpatialJoinRenderer::render()
    {
        launchParamsBuffer.upload(&launchParams,1);

        OPTIX_CHECK(optixLaunch(/*! pipeline we're launching launch: */
                            pipeline,stream,
                            /*! parameters and SBT */
                            launchParamsBuffer.d_pointer(),
                            launchParamsBuffer.sizeInBytes,
                            &sbt,
                            /*! dimensions of the launch: */
                            launchParams.dimension_x, // 2^16
                            1,
                            1
                            ));
        // sync - make sure the frame is rendered before we download and
        // display (obviously, for a high-performance application you
        // want to use streams and double-buffering, but for this simple
        // example, this will have to do)
        CUDA_SYNC_CHECK();
    }
    

    void SpatialJoinRenderer::downloadResult(int* h_result)
    {
        resultBuffer.download(h_result, 1);
        // download once
        resultBuffer.free();
        // free all the parameters
        launchParamsBuffer.free();
    }

}