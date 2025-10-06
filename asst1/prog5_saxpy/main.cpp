#include <stdio.h>
#include <algorithm>

#include "CycleTimer.h"
#include "saxpy_ispc.h"

extern void saxpySerial(int N, float a, float* X, float* Y, float* result);


// return GB/s
static float
toBW(int bytes, float sec) {
    return static_cast<float>(bytes) / (1024. * 1024. * 1024.) / sec;
}

static float
toGFLOPS(int ops, float sec) {
    return static_cast<float>(ops) / 1e9 / sec;
}

static void verifyResult(int N, float* result, float* gold) {
    for (int i=0; i<N; i++) {
        if (result[i] != gold[i]) {
            printf("Error: [%d] Got %f expected %f\n", i, result[i], gold[i]);
        }
    }
}

using namespace ispc;
namespace ispc {
    extern void saxpy_ispc_withtasks_stream(int N, float scale, float* X, float* Y, float* result);
}


int main() {

    const unsigned int N = 20 * 1000 * 1000; // 20 M element vectors (~80 MB)
    const unsigned int TOTAL_BYTES = 4 * N * sizeof(float);
    // const unsigned int TOTAL_BYTES_STREAM = 3 * N * sizeof(float); // stream version doesn't read result
    const unsigned int TOTAL_FLOPS = 2 * N;

    float scale = 2.f;

    float* arrayX = new float[N];
    float* arrayY = new float[N];
    float* resultSerial = new float[N];
    float* resultISPC = new float[N];
    float* resultTasks = new float[N];
    float *resultTasks_stream = nullptr;
    if (posix_memalign((void**)&resultTasks_stream, 32, N * sizeof(float)) != 0) {
        fprintf(stderr, "posix_memalign failed\n");
        exit(1);
    }

    // initialize array values
    for (unsigned int i=0; i<N; i++)
    {
        arrayX[i] = i;
        arrayY[i] = i;
        resultSerial[i] = 0.f;
        resultISPC[i] = 0.f;
        resultTasks[i] = 0.f;
        resultTasks_stream[i] = 0.f;
    }

    //
    // Run the serial implementation. Repeat three times for robust
    // timing.
    //
    double minSerial = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime =CycleTimer::currentSeconds();
        saxpySerial(N, scale, arrayX, arrayY, resultSerial);
        double endTime = CycleTimer::currentSeconds();
        minSerial = std::min(minSerial, endTime - startTime);
    }

// printf("[saxpy serial]:\t\t[%.3f] ms\t[%.3f] GB/s\t[%.3f] GFLOPS\n",
    //       minSerial * 1000,
    //       toBW(TOTAL_BYTES, minSerial),
    //       toGFLOPS(TOTAL_FLOPS, minSerial));

    //
    // Run the ISPC (single core) implementation
    //
    double minISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        saxpy_ispc(N, scale, arrayX, arrayY, resultISPC);
        double endTime = CycleTimer::currentSeconds();
        minISPC = std::min(minISPC, endTime - startTime);
    }

    verifyResult(N, resultISPC, resultSerial);

    printf("[saxpy ispc]:\t\t[%.3f] ms\t[%.3f] GB/s\t[%.3f] GFLOPS\n",
           minISPC * 1000,
           toBW(TOTAL_BYTES, minISPC),
           toGFLOPS(TOTAL_FLOPS, minISPC));

    //
    // Run the ISPC (multi-core) implementation
    //
    double minTaskISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        saxpy_ispc_withtasks(N, scale, arrayX, arrayY, resultTasks);
        double endTime = CycleTimer::currentSeconds();
        minTaskISPC = std::min(minTaskISPC, endTime - startTime);
    }

    verifyResult(N, resultTasks, resultSerial);

    printf("[saxpy task ispc]:\t[%.3f] ms\t[%.3f] GB/s\t[%.3f] GFLOPS\n",
           minTaskISPC * 1000,
           toBW(TOTAL_BYTES, minTaskISPC),
           toGFLOPS(TOTAL_FLOPS, minTaskISPC));

    // 
    // Run the ISPC (multi-core + streaming store) implementation
    //
    double minTaskISPC_stream = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        saxpy_ispc_withtasks_stream(N, scale, arrayX, arrayY, resultTasks_stream);
        double endTime = CycleTimer::currentSeconds();
        minTaskISPC_stream = std::min(minTaskISPC_stream, endTime - startTime);
    }

    verifyResult(N, resultTasks_stream, resultSerial);

    printf("[saxpy task+stream ispc]:\t[%.3f] ms\t[%.3f] GB/s\t[%.3f] GFLOPS\n",
           minTaskISPC_stream * 1000,
           toBW(TOTAL_BYTES, minTaskISPC_stream),
           toGFLOPS(TOTAL_FLOPS, minTaskISPC_stream));

    printf("\t\t\t\t(%.2fx speedup from use of tasks)\n", minISPC/minTaskISPC);
    printf("\t\t\t\t(%.2fx speedup from use of tasks+stream)\n", minISPC/minTaskISPC_stream);
    //printf("\t\t\t\t(%.2fx speedup from ISPC)\n", minSerial/minISPC);
    //printf("\t\t\t\t(%.2fx speedup from task ISPC)\n", minSerial/minTaskISPC);

    delete[] arrayX;
    delete[] arrayY;
    delete[] resultSerial;
    delete[] resultISPC;
    delete[] resultTasks;

    return 0;
}
