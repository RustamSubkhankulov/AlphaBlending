Using AVX intrinsics functions to optimize AlphaBlending process
================================================================

Testing two versions
--------------------
AlphaBlending prog - patches one image on another, saves it in file and shows result in window using SFML

Configure
---------
1. File /src/alphablending/alpha_b_conf.h contains configuring definition - OPT
   <code> #define OPT </code>

  If this <code>OPT</code> is defined, wll be used optimized version of calculating algotitm with SSE instuctions, using xmm registers
  Else will be used algorithm without any optimizations
  
2. <code> static const int Evaluations_number = 100; </code> Number of times evaluations will be performed
   Calculating is performed multiple times, so random error does not affect the results

Using
-----
Example of using 

Note: firstly use 'make' to build project

Warning: patching image must be less size than background one

<code> /overlay pictures/Table.bmp pictures/Racket.bmp pictures/Result.bmp 200 300 </code>

Arguments:
1. Background image
2. Foreground image
3. Result image name ( where result will be saved )
4. Coordinates of patch image on the background from left top angle

Controls
--------

Press 'T' to show time, spent on calclations

Comparing
---------

Number of cycles of calculating - 4096

| Version          | TIME, sec |
| ---------------- | --------- |
| no optimization  | 5.218     | 
| SSE              | 1.390     |

Conclusion
----------

Increase in performance estimately 3,75 times - almost 4 times as was expected
Although SSE instructions performs calculations with 4 values at one time, increase is not 8 times. This probably causes by other instructions in algorithm, for example, memory access
