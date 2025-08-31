#include <math.h>
#include <stdio.h>
#include <immintrin.h>

void sqrtAvx2(int N, float initialGuess, float *values, float *output) {
    static const float kThreshold = 0.00001f;
    __m256 kThreshold_vec = _mm256_set1_ps(kThreshold);

    // int (`1000 0000 0000 0000 0000 0000 0000 0000`) -> float
    __m256 sign_mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000));

    // The binary representation of `-0.0f` is `1000 0000 0000 0000 0000 0000 0000 0000`.
    // __m256 sign_mask = _mm256_set1_ps(-0.0f);

    // Calculate the maximum length that can be divided by 8.
    // AVX2 can process 8 floats simultaneously
    int aligned_n = (N / 8) * 8;

    for (int i = 0; i < aligned_n; i += 8) {
        // Load 8 values from the input array
        __m256 x = _mm256_loadu_ps(&values[i]);
        // Initialize the guess
        __m256 guess = _mm256_set1_ps(initialGuess);

        // Compute the initial error
        __m256 error = _mm256_sub_ps(_mm256_mul_ps(_mm256_mul_ps(guess, guess), x), _mm256_set1_ps(1.0f));
        error = _mm256_andnot_ps(sign_mask, error);

        // Iterate until all elements converge
        while (_mm256_movemask_ps(_mm256_cmp_ps(error, kThreshold_vec, _CMP_GT_OQ)) != 0) {
            guess = _mm256_mul_ps(_mm256_set1_ps(0.5f), _mm256_sub_ps(_mm256_mul_ps(_mm256_set1_ps(3.0f), guess), _mm256_mul_ps(x, _mm256_mul_ps(guess, _mm256_mul_ps(guess, guess)))));
            error = _mm256_sub_ps(_mm256_mul_ps(_mm256_mul_ps(guess, guess), x), _mm256_set1_ps(1.0f));
            error = _mm256_andnot_ps(sign_mask, error);
        }

        // Store the result
        _mm256_storeu_ps(&output[i], _mm256_mul_ps(x, guess));
    }

    // Use the scalar algorithm to process the remaining elements (the part with less than 8 elements).
    for (int i = aligned_n; i < N; i++) {
        float x = values[i];
        float guess = initialGuess;
        float error = fabs(guess * guess * x - 1.f);

        while (error > kThreshold) {
            guess = (3.f * guess - x * guess * guess * guess) * 0.5f;
            error = fabs(guess * guess * x - 1.f);
        }

        output[i] = x * guess;
    }
}
