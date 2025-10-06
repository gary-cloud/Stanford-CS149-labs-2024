initial result:
```bash
$ ./kmeans 
Reading data.dat...
Running K-means with: M=1000000, N=100, K=3, epsilon=0.100000
[computeAssignments time]: 3512.843 ms
[computeCentroids time]: 784.136 ms
[computeCost time]: 1096.773 ms
[Total Time]: 5393.845 ms
```

And we use `std::thread` to parallelize `computeAssignments` function:
```bash
$ ./kmeans 
Reading data.dat...
Running K-means with: M=1000000, N=100, K=3, epsilon=0.100000
[computeAssignments time]: 706.243 ms
[computeCentroids time]: 775.850 ms
[computeCost time]: 1064.751 ms
[Total Time]: 2546.966 ms
```

So, we get a speedup of about 2.11x finally.