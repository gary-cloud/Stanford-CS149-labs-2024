
void saxpySerial(int N,
                       float scale,
                       float X[],
                       float Y[],
                       float result[])
{

    for (int i=0; i<N; i++) {
        // 1. read X
        // 2. read Y
        // 3. read-for-ownership(result), RFO, take result to cache line
        // 4. write-back(result)
        result[i] = scale * X[i] + Y[i];
    }
}

