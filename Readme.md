<div align=center>

# Search Algorithm

</div>



## TOC
- [Installation](#installation)
- [Main Idea](#nain-idea)
- [Time complexity](#time-complexity)
  - [Deep Functions](#deep-functions)
  - [Short Table](#short-table)
- [Results](#results)



## Installation
### Download dependencies
[vcpkg](https://vcpkg.io/en/)
```bash
vcpkg install --x-install-root=./vendor
```

### Compile and Run
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build && ./build/search
```



## Main Idea
Think of it like an **address book with letter tabs (`A`–`Z`)**: instead of flipping through millions of pages one by one with binary search, you calculate the exact tab a number belongs to and jump straight there.

### 1. Build Index (`build_index`) — $O(n)$
We organize raw, unsorted numbers into tiny sorted buckets without ever sorting the whole list at once:

1. **Measure the Data (`min_val`, `max_val`)**:
   - Scan the list once to find the smallest and largest numbers (`range_span = max_val - min_val`).
2. **Divide & Count (`seg_counts`)**:
   - Split the number range into `num_segments` equal slices (between 1,000 and 65,536) and count how many numbers land in each slice.
3. **Create a Shortcut Formula (`slopes`, `intercepts`)**:
   - Using those slice counts, draw a straight line (`slope * x + intercept`) for each slice that maps any number directly to its target bucket ID (`bucket_clasificator`). Crowded slices get spread across many buckets; empty slices get compressed. This function must be **monotonically non-decreasing** for corect data search.
4. **Group & Sort Locally (`bucket_start`, `counts`)**:
   - Calculate each bucket's starting position in memory, place every number into its assigned bucket, and sort each tiny bucket on its own (using fast insertion sort for small buckets and `std::sort` for larger ones). Because the buckets are already in order, the whole dataset ends up sorted.

---

### 2. Search for Nearest Value (`find_nearest_bucketed`) — $O(1)$ Expected
1. **Jump to the Bucket**:
   - Run `bucket_clasificator(target)` to instantly calculate the predicted bucket `b_id` in $O(1)$ math operations.
2. **Check the $\pm 1$ Neighborhood**:
   - Look at `b_id - 1`, `b_id`, and `b_id + 1` (skipping empty buckets) to pick the bucket whose range covers `target`. *(If all three are empty, fall back to standard binary search.)*
3. **Pick the Closest Number**:
   - Run a tiny binary search inside that single bucket and return whichever neighbor (`left` or `right`) is closer to `target`.
4. **Speed**:
   - Because `num_buckets` equals `data.size()`, each bucket only holds a few numbers on average—turning an $O(\log n)$ search across millions of items into a near-instant $O(1)$ lookup.



## Time complexity
### Deep Functions
#### inline double fast_abs(double x) const;
##### Function
$f(n) = 2 \implies \mathcal{O}(1)$
#### inline long long find_segment(double value) const;
##### Function
$f(n) = 7 \implies \mathcal{O}(1)$
#### long long bucket_clasificator(double val) const, CLASIFY_FAST;
##### Function
$f(n) = 16 \implies \mathcal{O}(1)$
#### void build_index(long long num_buckets = 0, long long target_segment_size = 64);
##### Function
$
  n \implies \text{size of data}  \\  
  b = n \implies \text{size of buckets}  \\  
  s \in (1000, 65536) \implies \text{size of segments}  \\  
  k \approx \frac{n}{b} \approx 1 \implies \text{elements in bucket ideal data distribution aproach 1}  \\  
$
$f(n, b, s) = 61n + 24s + b(5 + klog_{2}{k}) - 73\\$
$f(n, b, s) = 61n + 24s + 6b - 73 \implies \mathcal{O}(n)\\$
#### void clear();
##### Function
$f(n) = 5$
#### long long find_nearest(double target) const;
##### Function
$
  n \implies \text{size of data} \\ 
$
$f(n) = nlog_{2}{n} + 18$
#### long long find_nearest_bucketed(double target) const;
##### Function
$
  n \implies \text{size of data}  \\  
  b = n \implies \text{size of buckets}  \\  
  k \approx \frac{n}{b} \approx 1 \implies \text{elements in bucket ideal data distribution aproach 1}  \\  
$
$f(n, k) = 10\log_{2}{k} + 54\\$
$f(n) = 64 \implies \mathcal{O}(1)\\$

### Short Table
| Function / Component | Time Complexity | Space Complexity |
| :--- | :--- | :--- |
| **`build_index`** | $\mathcal{O}(n)$ | $\mathcal{O}(n)$ |
| **`find_segment`** | $\mathcal{O}(1)$ | $\mathcal{O}(1)$ |
| **`bucket_clasificator`** | $\mathcal{O}(1)$ | $\mathcal{O}(1)$ |
| **`find_nearest_bucketed`** | $\mathcal{O}(1) + \mathcal{O}(\log k)$ | $\mathcal{O}(1)$ |
| **`find_nearest` (Fallback)** | $\mathcal{O}(\log n)$ | $\mathcal{O}(1)$ |



## Results 
```txt
========================
=== Seach Algorithms ===
========================
=== Creating Benchmark Envitoments ===
=== Creating Benchmark Structure ===
=== Linear Distribution Enviroment ===
=== Prepare Data

Generating Data
[=======================================>] [00m:05s<00m:00s]                                                        
=== Saving Data to Binary File ===
Data saved to data/data.bin

size: 100000000

size: 100000000
=== Prepare Testing Data
Generating Queries for Standard Case

=== Binary Search Setting Data ===
=== Binary Search Prepare Structure ===

Sorting Data
Binary Search Prepare Time: 22471.492 ms
=== Binary Search Running Benchmarks (100000000 queries) ===
Binary Search Querry Time: 83745.340 ms
=== Bucketed Search Setting Data ===
=== Bucketed Search Prepare Structure ===
Bucketed Search Prepare Time: 11513.544 ms
=== Bucketed Search Running Benchmarks (100000000 queries) ===
Bucketed Search Querry Time: 21937.194 ms
=== Results ===
=== Results for Binary Search ===
Querry Binary Search:
  Total Time:   83745.340 ms
  Per Query:    0.000837 ms
Querry + Prepare Binary Search:
  Total Time:    106216.831 ms
  Per Query:     0.001062 ms
=== Results for Bucketed Search ===
Querry Bucketed Search:
  Total Time:   21937.194 ms
  Per Query:    0.000219 ms
Querry + Prepare Bucketed Search:
  Total Time:    33450.738 ms
  Per Query:     0.000335 ms

=== Performance Leaderboard & Gaps ===
#1  | Bucketed Search                | Query Time:  21937.194 ms | Total Time:  33450.738 ms
     └─ vs #2: FASTER by 3.82x (saved 61808.146 ms)
     └─ Dominates from query #1 (better build time & query time)
#2  | Binary Search                  | Query Time:  83745.340 ms | Total Time: 106216.831 ms

SUCCESS: Checksums match perfectly!
Binary Search Checksum: 5000066910815975
Bucketed Search Checksum: 5000066910815975
=== Binary Search Info ===
=== Bucketed Search Info ===

=== Learned Index Parameters (O(1) Piecewise Linear) ===
  Dataset Size:        100000000
  Min / Max Value:     -19999998.0463 / 19999999.5734
  Range Span:          39999997.6196
  Segments Count:      65536
  Total Buckets:       100000001
===============================================


=== Index Quality & Query Cost Analysis ===
  Total Buckets:       100000000
  Non-empty Buckets:   63223798 (63.22%)
  Empty Buckets:       36776202
  Avg Elements/Bucket: 1.58
  Max Bucket Size:     11 elements
  Max Elements Scanned per Query (Worst Case ±1 window): ~33
===========================================


=== Classifier Graph: bucket_clasificator(x) [Segments: 65536] ===
  99999999 |                                                                              **
  95652173 |                                                                          ****  
  91304347 |                                                                       ***      
  86956521 |                                                                   ****         
  82608695 |                                                                ***             
  78260869 |                                                             ***                
  73913043 |                                                         ****                   
  69565217 |                                                      ***                       
  65217391 |                                                  ****                          
  60869565 |                                               ***                              
  56521739 |                                           ****                                 
  52173913 |                                        ***                                     
  47826086 |                                     ***                                        
  43478260 |                                 ****                                           
  39130434 |                              ***                                               
  34782608 |                          ****                                                  
  30434782 |                       ***                                                      
  26086956 |                   ****                                                         
  21739130 |                ***                                                             
  17391304 |             ***                                                                
  13043478 |         ****                                                                   
   8695652 |      ***                                                                       
   4347826 |  ****                                                                          
         0 |**                                                                              
           +--------------------------------------------------------------------------------
            -19999998.05                          0.76                           19999999.57

=== Step Distribution Enviroment ===
=== Prepare Data

Generating Data

Generating Skewed Data with Outliers
[====================================>   ] [00m:02s<00m:00s]                                                        
=== Saving Data to Binary File ===
Data saved to data/steps.data.bin

size: 100000000

size: 100000000
=== Prepare Testing Data
Generating Queries for Offset Case

Generating Skewed Data with Outliers

=== Binary Search Setting Data ===
=== Binary Search Prepare Structure ===

Sorting Data
Binary Search Prepare Time: 21136.727 ms
=== Binary Search Running Benchmarks (100000000 queries) ===
Binary Search Querry Time: 85897.530 ms
=== Bucketed Search Setting Data ===
=== Bucketed Search Prepare Structure ===
Bucketed Search Prepare Time: 7950.398 ms
=== Bucketed Search Running Benchmarks (100000000 queries) ===
Bucketed Search Querry Time: 19093.914 ms
=== Results ===
=== Results for Binary Search ===
Querry Binary Search:
  Total Time:   85897.530 ms
  Per Query:    0.000859 ms
Querry + Prepare Binary Search:
  Total Time:    107034.256 ms
  Per Query:     0.001070 ms
=== Results for Bucketed Search ===
Querry Bucketed Search:
  Total Time:   19093.914 ms
  Per Query:    0.000191 ms
Querry + Prepare Bucketed Search:
  Total Time:    27044.313 ms
  Per Query:     0.000270 ms

=== Performance Leaderboard & Gaps ===
#1  | Bucketed Search                | Query Time:  19093.914 ms | Total Time:  27044.313 ms
     └─ vs #2: FASTER by 4.50x (saved 66803.615 ms)
     └─ Dominates from query #1 (better build time & query time)
#2  | Binary Search                  | Query Time:  85897.530 ms | Total Time: 107034.256 ms

SUCCESS: Checksums match perfectly!
Binary Search Checksum: 4999945721348158
Bucketed Search Checksum: 4999945721348158
=== Binary Search Info ===
=== Bucketed Search Info ===

=== Learned Index Parameters (O(1) Piecewise Linear) ===
  Dataset Size:        100000000
  Min / Max Value:     -10.0000 / 310.0000
  Range Span:          320.0000
  Segments Count:      65536
  Total Buckets:       100000001
===============================================


=== Index Quality & Query Cost Analysis ===
  Total Buckets:       100000000
  Non-empty Buckets:   63217569 (63.22%)
  Empty Buckets:       36782431
  Avg Elements/Bucket: 1.58
  Max Bucket Size:     11 elements
  Max Elements Scanned per Query (Worst Case ±1 window): ~33
===========================================


=== Classifier Graph: bucket_clasificator(x) [Segments: 65536] ===
  99999999 |                                                                               *
  95652173 |                                                                              * 
  91304347 |                                                                             *  
  86956521 |                                                                            *   
  82608695 |                                                                            |   
  78260869 |                                                                           *    
  73913043 |                                                      *********************     
  69565217 |                                                     *                          
  65217391 |                                                    *                           
  60869565 |                                                    |                           
  56521739 |                                                   *                            
  52173913 |                              *********************                             
  47826086 |                             *                                                  
  43478260 |                            *                                                   
  39130434 |                            |                                                   
  34782608 |                           *                                                    
  30434782 |                          *                                                     
  26086956 |     *********************                                                      
  21739130 |    *                                                                           
  17391304 |    |                                                                           
  13043478 |   *                                                                            
   8695652 |  *                                                                             
   4347826 | *                                                                              
         0 |*                                                                               
           +--------------------------------------------------------------------------------
            -10.00                               150.00                               310.00
```

