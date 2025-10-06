// saxpy.ispc
// Declare C function to be called from ISPC (C linkage, no ISPC name mangling)
extern "C" void saxpy_stream_chunk(uniform int n,
                                   uniform float scale,
                                   uniform const float* x,
                                   uniform const float* y,
                                   uniform float* result);

export void saxpy_ispc(uniform int N,
                       uniform float scale,
                            uniform float X[],
                            uniform float Y[],
                            uniform float result[])
{
    foreach (i = 0 ... N) {           
        result[i] = scale * X[i] + Y[i];
    }
}

task void saxpy_ispc_task(uniform int N,
                               uniform int span,
                               uniform float scale,
                               uniform float X[], 
                               uniform float Y[],
                               uniform float result[])
{

    uniform int indexStart = taskIndex * span;
    uniform int indexEnd = min(N, indexStart + span);

    foreach (i = indexStart ... indexEnd) {
        result[i] = scale * X[i] + Y[i];
    }
}

export void saxpy_ispc_withtasks(uniform int N,
                               uniform float scale,
                               uniform float X[],
                               uniform float Y[],
                               uniform float result[])
{

    uniform int span = N / 64;  // 64 tasks

    launch[N/span] saxpy_ispc_task(N, span, scale, X, Y, result);
}

// Task function (unchanged, processes range according to start/span)
// Note: result base address must already be 32-byte aligned (posix_memalign)
task void saxpy_ispc_task_stream(uniform int N,
                                 uniform int span,
                                 uniform float scale,
                                 uniform float X[],
                                 uniform float Y[],
                                 uniform float result[])
{
    // taskIndex is a built-in ISPC variable indicating the current task number
    uniform int start = taskIndex * span;
    uniform int end   = min(N, start + span);
    uniform int len   = end - start;

    if (len > 0) {
        // Call C/AVX2 core: pass result + start
        saxpy_stream_chunk(len, scale, X + start, Y + start, result + start);
    }
}

// Exported function: create tasks at ISPC layer (computation delegated to C kernel)
export void saxpy_ispc_withtasks_stream(uniform int N,
                                        uniform float scale,
                                        uniform float X[],
                                        uniform float Y[],
                                        uniform float result[])
{
    // Target number of tasks (can adjust)
    uniform int desired_tasks = 16;

    // 1) Compute base span (round up)
    uniform int base_span = (N + desired_tasks - 1) / max(desired_tasks, 1);

    // 2) Adjust span to multiple of 8 (ensures start = taskIndex * span is multiple of 8)
    //    8 floats == 32 bytes alignment
    uniform int span = ((base_span + 7) / 8) * 8;
    if (span < 8) span = 8; // at least 8, avoid zero span

    // 3) Recompute actual task count to avoid empty tasks
    uniform int actual_tasks = (N + span - 1) / span;
    if (actual_tasks < 1) actual_tasks = 1;

    // 4) Launch actual number of tasks
    launch[actual_tasks] saxpy_ispc_task_stream(N, span, scale, X, Y, result);
}
