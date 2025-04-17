#include <cuda_runtime.h>
#include <iostream>

int main() {
    // Check runtime version
    int runtime_version = 0;
    cudaRuntimeGetVersion(&runtime_version);

    int major = runtime_version / 1000;
    int minor = (runtime_version % 1000) / 10;

    std::cout << "CUDA Runtime Version: " << major << "." << minor << std::endl;

    // Check device compute capability
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);

    std::cout << "GPU: " << prop.name << std::endl;
    std::cout << "Compute Capability: " << prop.major << "." << prop.minor << std::endl;

    // Try cudaMallocAsync (only if version and compute capability are good)
    if (runtime_version >= 11020 && prop.major >= 7) {
        std::cout << "Attempting cudaMallocAsync test..." << std::endl;

        void* ptr = nullptr;
        cudaStream_t stream;
        cudaStreamCreate(&stream);

        cudaError_t err = cudaMallocAsync(&ptr, 1024, stream);

        if (err == cudaSuccess) {
            std::cout << "cudaMallocAsync() succeeded!" << std::endl;
            cudaFreeAsync(ptr, stream);
        } else {
            std::cout << "cudaMallocAsync() failed: "
                      << cudaGetErrorString(err) << std::endl;
        }

        cudaStreamDestroy(stream);
    } else {
        std::cout << "cudaMallocAsync() not supported on this system." << std::endl;
    }

    return 0;
}
