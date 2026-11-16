<div align=center>

# Search Algorithm

</div>



## TOC
- [Installation](#installation)
- [Main Idea](#nain-idea)
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
Binary Search Prepare Time: 22663.662 ms
=== Binary Search Running Benchmarks (100000000 queries) ===
Binary Search Querry Time: 89397.937 ms
=== Bucketed Search Setting Data ===
=== Bucketed Search Prepare Structure ===
Bucketed Search Prepare Time: 8475.770 ms
=== Bucketed Search Running Benchmarks (100000000 queries) ===
Bucketed Search Querry Time: 24224.233 ms
=== Results ===
=== Results for Binary Search ===
Querry Binary Search:
  Total Time:   89397.937 ms
  Per Query:    0.000894 ms
Querry + Prepare Binary Search:
  Total Time:    112061.599 ms
  Per Query:     0.001121 ms
=== Results for Bucketed Search ===
Querry Bucketed Search:
  Total Time:   24224.233 ms
  Per Query:    0.000242 ms
Querry + Prepare Bucketed Search:
  Total Time:    32700.003 ms
  Per Query:     0.000327 ms

=== Performance Leaderboard & Gaps ===
#1  | Bucketed Search                | Query Time:  24224.233 ms | Total Time:  32700.003 ms
     └─ vs #2: FASTER by 3.69x (saved 65173.704 ms)
     └─ Dominates from query #1 (better build time & query time)
#2  | Binary Search                  | Query Time:  89397.937 ms | Total Time: 112061.599 ms

SUCCESS: Checksums match perfectly!
Binary Search Checksum: 4999930237504824
Bucketed Search Checksum: 4999930237504824
=== Binary Search Info ===
=== Bucketed Search Info ===

=== Learned Index Parameters (O(1) Piecewise Linear) ===
  Dataset Size:        100000000
  Min / Max Value:     -19999999.9960 / 19999999.8296
  Range Span:          39999999.8255
  Segments Count:      65536
  Total Buckets:       100000000
===============================================


=== Index Quality & Query Cost Analysis ===
  Total Buckets:       100000000
  Non-empty Buckets:   63222656 (63.22%)
  Empty Buckets:       36777344
  Avg Elements/Bucket: 1.58
  Max Bucket Size:     10 elements
  Max Elements Scanned per Query (Worst Case ±1 window): ~30
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
            -20000000.00                         -0.08                           19999999.83

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
Binary Search Prepare Time: 21029.730 ms
=== Binary Search Running Benchmarks (100000000 queries) ===
Binary Search Querry Time: 76975.719 ms
=== Bucketed Search Setting Data ===
=== Bucketed Search Prepare Structure ===
Bucketed Search Prepare Time: 7364.118 ms
=== Bucketed Search Running Benchmarks (100000000 queries) ===
Bucketed Search Querry Time: 22407.864 ms
=== Results ===
=== Results for Binary Search ===
Querry Binary Search:
  Total Time:   76975.719 ms
  Per Query:    0.000770 ms
Querry + Prepare Binary Search:
  Total Time:    98005.450 ms
  Per Query:     0.000980 ms
=== Results for Bucketed Search ===
Querry Bucketed Search:
  Total Time:   22407.864 ms
  Per Query:    0.000224 ms
Querry + Prepare Bucketed Search:
  Total Time:    29771.982 ms
  Per Query:     0.000298 ms

=== Performance Leaderboard & Gaps ===
#1  | Bucketed Search                | Query Time:  22407.864 ms | Total Time:  29771.982 ms
     └─ vs #2: FASTER by 3.44x (saved 54567.855 ms)
     └─ Dominates from query #1 (better build time & query time)
#2  | Binary Search                  | Query Time:  76975.719 ms | Total Time:  98005.450 ms

SUCCESS: Checksums match perfectly!
Binary Search Checksum: 5000148470307501
Bucketed Search Checksum: 5000148470307501
=== Binary Search Info ===
=== Bucketed Search Info ===

=== Learned Index Parameters (O(1) Piecewise Linear) ===
  Dataset Size:        100000000
  Min / Max Value:     -10.0000 / 310.0000
  Range Span:          320.0000
  Segments Count:      65536
  Total Buckets:       100000000
===============================================


=== Index Quality & Query Cost Analysis ===
  Total Buckets:       100000000
  Non-empty Buckets:   63219177 (63.22%)
  Empty Buckets:       36780823
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

