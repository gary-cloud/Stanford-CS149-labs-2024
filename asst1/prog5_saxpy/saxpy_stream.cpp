// saxpy_stream.cpp
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <algorithm>

extern "C" void saxpy_stream_chunk(int n,
                                   float scale,
                                   const float* x,
                                   const float* y,
                                   float* result)
{
    const int W = 8; // AVX2: 8 floats per vector
    int i = 0;

    __m256 vscale = _mm256_set1_ps(scale);

    // Main loop: vectorized + non-temporal store
    for (; i <= n - W; i += W) {
        __m256 vx = _mm256_loadu_ps(x + i);   // Use _mm256_load_ps if 32B aligned
        __m256 vy = _mm256_loadu_ps(y + i);
        // r = scale * x + y (FMA)
        __m256 vr = _mm256_fmadd_ps(vscale, vx, vy);
        // Non-temporal store: avoid Read-For-Ownership (RFO)
        _mm256_stream_ps(result + i, vr);     // Use aligned store if 32B aligned
    }

    // Ensure visibility of streaming stores
    _mm_sfence();
}
