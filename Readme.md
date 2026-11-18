<div align=center>

# Search Algorithm

</div>



## About
A high-performance search structure that deliberately prioritizes speed over memory efficiency. It uses additional memory for a learned bucket index to minimize search operations and accelerate queries. Achieves approximately **3–5× overall speedup** compared to conventional sorting followed by binary search, with around **5×** faster query execution in tested benchmarks.  
**Core philosophy**: Spend memory to save computation time.



## TOC
- [About](#about)
- [Installation](#installation)
- [Main Idea](#main-idea)
  - [Build Index](#build-index)
  - [Search for Nearest Value](#search-for-nearest-value)
  - [Search Performance](#search-performance)
- [Time Complexity](#time-complexity)
  - [Deep Functions](#deep-functions)
  - [Short Table](#short-table)
- [Memory Structure](#memory-structure)
- [Final Concept](#final-concept)
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
Think of it like an **address book with letter tabs (`A`–`Z`)**: instead of searching through millions of entries using binary search, you calculate the approximate location of a number and jump directly to its bucket.
The system uses a **piecewise-linear learned index** to predict bucket IDs. It divides the value range into segments, learns a linear mapping for each segment, and distributes the original data into buckets.
Each bucket stores references to the original data, sorted locally by value. This allows the search algorithm to locate nearby values without sorting or searching the entire dataset.

<a id="build-index"></a>
### 1. Build Index (`build_index`) — $O(n+b+s)$ Expected
We organize raw, unsorted numbers into locally sorted buckets without globally sorting the entire dataset:

1. **Measure the Data (`min_val`, `max_val`)**:
   - Scan the original dataset once to find the smallest and largest values.
   - Calculate the value range (`range_span = max_val - min_val`).
   - Calculate the inverse range (`inv_span`) for faster normalization.

2. **Divide & Count (`seg_counts`)**:
   - Divide the numerical range into `num_segments` equally sized intervals.
   - The number of segments is determined by the dataset size and `target_segment_size`, constrained between 1,000 and 65,536.
   - Count how many elements belong to each segment.

3. **Create a Shortcut Formula (`slopes`, `intercepts`)**:
   - Calculate a linear mapping for every segment using its cumulative element distribution.
   - Each segment receives its own slope and intercept.
   - The resulting formula is: $bucket(x)=slope_i\cdot x+intercept_i$
   - Crowded segments receive a larger bucket range, while sparse segments receive a smaller one.
   - The classifier must remain monotonically non-decreasing for the bucket ordering to be valid.

4. **Calculate Bucket Boundaries (`bucket_start`)**:
   - Classify every element using `CLASSIFY_FAST`.
   - Count the number of elements assigned to each bucket.
   - Calculate cumulative bucket boundaries using a prefix sum.
   - These boundaries determine where each bucket begins and ends in the auxiliary index.

5. **Distribute Elements (`counts`)**:
   - Place the original data indices into their corresponding buckets.
   - The `counts` array stores original data indices, not the values themselves.
   - This avoids copying the original elements and allows the buckets to reference the original dataset.

6. **Sort Locally**:
   - Sort each bucket independently.
   - Buckets containing two elements use a direct comparison.
   - Buckets containing up to 32 elements use insertion sort.
   - Larger buckets use `std::sort`.
   - Because the bucket classifier is monotonic and bucket boundaries preserve the numerical ordering, the resulting bucket sequence represents a globally sorted index.

The original dataset remains unchanged.

<a id="search-for-nearest-value"></a>
### 2. Search for Nearest Value (`find_nearest_bucketed`) — $O(\log k)$ Expected
1. **Predict the Bucket**:
   - Run `bucket_clasificator(target)` to calculate the predicted bucket ID in $O(1)$.
   - The classifier selects the appropriate segment and evaluates its linear prediction formula.

2. **Locate the Bucket Boundaries**:
   - Use `bucket_start[b]` and `bucket_start[b+1]` to determine the selected bucket's range.
   - This provides direct access to the corresponding region of the auxiliary index.

3. **Search Inside the Bucket**:
   - Run `lower_pos()` to find the insertion position of the target.
   - For buckets containing at most 16 elements, use a linear scan.
   - Larger buckets use `std::lower_bound`.
   - The search operates on the sorted original-data indices stored in `counts`.

4. **Validate the Search Result**:
   - Check whether the neighboring values satisfy the expected ordering around the target.
   - If the predicted bucket does not contain the appropriate search boundary, execute the fallback search.

5. **Fallback Search (`search_window`)**:
   - Search the predicted bucket and adjacent buckets.
   - The fallback uses the combined index interval covering up to three neighboring buckets.
   - This protects the search against prediction errors and empty buckets, provided the classifier's monotonic ordering assumptions hold.

6. **Pick the Closest Number**:
   - Compare the values immediately before and after the insertion position.
   - Return the original dataset index corresponding to the nearest value.

### Search Performance
The main advantage is that the algorithm does not need to search the entire dataset.
Instead, it calculates an approximate location in constant time and performs a localized search.
For small buckets, the search is effectively constant-time in practice.
However, the theoretical complexity depends on bucket occupancy:

$
T(k)=
\begin{cases}
O(k), & k\leq16\\
O(\log k), & k>16
\end{cases}
$

The complete search also includes constant-time classification and boundary access.
For bounded bucket occupancy: $T(n)=O(1)$

This is a practical expected-case property, not a universal worst-case guarantee.



## Time Complexity

### Deep Functions
#### `inline double fast_abs(double x) const;`
Function: $f(n)=2\implies\mathcal{O}(1)$  
Performs bitwise absolute-value extraction on a `double`.

#### `inline long long find_segment(double value) const;`
Function: $f(n)=7\implies\mathcal{O}(1)$  
Calculates the segment corresponding to a numerical value using normalization and direct indexing.

#### `long long bucket_clasificator(double val) const`
Function: $f(n)=16\implies\mathcal{O}(1)$  
Calculates the bucket ID using a piecewise-linear function.  
The operation consists of:
- Segment identification.
- Slope and intercept lookup.
- Linear prediction.
- Bucket boundary clamping.

#### `void build_index(long long num_buckets = 0, long long target_segment_size = 64);`
Definitions:   
$n=\text{number of elements in the dataset}$  
$b=\text{number of buckets}$  
$s=\text{number of segments}$
$k_i=\text{number of elements in bucket }i$  

The construction consists of:
1. Finding minimum and maximum values: $O(n)$
2. Counting segment elements: $O(n+s)$
3. Generating piecewise-linear parameters: $O(s)$
4. Counting and distributing elements into buckets: $O(n+b)$
5. Sorting individual buckets: $O\left(\sum_{i=0}^{b-1}k_i\log k_i\right)$

Therefore: $T(n,b,s)=O\left(n+b+s+\sum_{i=0}^{b-1}k_i\log k_i\right)$  
Since: $\sum_{i=0}^{b-1}k_i=n$  
For approximately constant bucket occupancy: $k_i\approx\frac{n}{b}$  
The construction becomes: $T(n,b,s)=O(n+b+s)$  
When $b=n$ and $s\leq65536$: $T(n)=O(n)$  

This assumes bounded average bucket occupancy and a bounded segment count.

#### `void clear();`
Function: $f(n)=5$
Clears the vectors and releases their logical contents.  
Time complexity: $O(n)$  
The actual cost depends on the number of allocated elements and the implementation of the containers.

#### `inline long long lower_pos(double target, long long lo, long long hi) const;`
Let: $k=hi-lo$  
For small ranges: $k\leq16\implies O(k)$  
For larger ranges: $O(\log k)$  
Uses a linear scan for small buckets and binary search for larger buckets.

#### `inline long long search_window(double target, long long lo, long long hi) const;`
Function: $O(\log k)$  
Finds the nearest value within a specified range of the auxiliary index, assuming the range is sorted.  

#### `long long find_nearest_bucketed(double target) const;`\
##### Function
Definitions:  
$n=\text{number of elements}$  
$b=n=\text{number of buckets}$  
$k=\text{occupancy of the searched bucket}$  

The normal search path consists of:  
- Bucket classification: $O(1)$.
- Bucket boundary lookup: $O(1)$.
- Local search: $O(\log k)$, or $O(k)$ for small buckets.
- Neighbor comparison: $O(1)$.

Therefore: $T(k)=O(\log k)$
For small, bounded bucket occupancy: $T(k)=O(1)$  

The fallback searches a window containing up to three adjacent buckets.  
If their combined occupancy is $k_w$: $T_{\text{fallback}}=O(\log k_w)$  
The theoretical worst case is: $T(n)=O(\log n)$  
when a bucket or fallback window contains a large fraction of the dataset.



## Short Table
| Function / Component | Time Complexity | Space Complexity |
| :--- | :--- | :--- |
| **`build_index`** | $O(n+b+s+\sum k_i\log k_i)$ | $O(n+b+s)$ |
| **`find_segment`** | $O(1)$ | $O(1)$ |
| **`bucket_clasificator`** | $O(1)$ | $O(1)$ |
| **`lower_pos`** | $O(\log k)$ | $O(1)$ |
| **`search_window`** | $O(\log k_w)$ | $O(1)$ |
| **`find_nearest_bucketed`** | $O(\log k)$ expected for bounded occupancy | $O(1)$ |
| **`find_nearest` (Binary Search)** | $O(\log n)$ | $O(1)$ |
| **`clear`** | $O(1)$ | $O(1)$ auxiliary |




## Memory Structure
The index uses four main auxiliary structures:  
| Structure | Purpose | Space |
| :--- | :--- | :--- |
| `slopes` | Segment linear coefficients | $O(s)$ |
| `intercepts` | Segment linear offsets | $O(s)$ |
| `bucket_start` | Bucket boundaries | $O(b)$ |
| `counts` | Original data indices, sorted by bucket | $O(n)$ |

The original dataset is stored separately in `data`.
Total memory complexity: $S(n,b,s)=O(n+b+s)$  
For the default configuration, where $b=n$: $S(n)=O(n)$  



## Final Concept
The central idea is to replace global binary search with **direct mathematical classification followed by localized searching**.  
Instead of sorting the entire dataset and searching through all its elements, the algorithm:
1. Learns the distribution of the dataset.
2. Constructs a piecewise-linear classifier.
3. Maps values directly to buckets.
4. Organizes original data indices into locally sorted groups.
5. Searches only the relevant region.
The result is a learned bucket index with linear construction under bounded bucket occupancy and near-constant practical query performance.  
**The key optimization is not eliminating sorting entirely, but replacing one global sorting operation with many small, independent sorting operations while using a learned classifier to navigate between them.**



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
[=======================================>] [00m:02s<00m:00s]                                                                                                                                                                                
=== Saving Data to Binary File ===
Data saved to data/data.bin

size: 100000000

size: 100000000
=== Prepare Testing Data
Generating Queries for Standard Case

=== Binary Search Setting Data ===
=== Binary Search Prepare Structure ===

Sorting Data
Binary Search Prepare Time: 10933.735 ms
=== Binary Search Running Benchmarks (100000000 queries) ===
Binary Search Querry Time: 51306.989 ms
=== Bucketed Search Setting Data ===
=== Bucketed Search Prepare Structure ===
Bucketed Search Prepare Time: 5768.052 ms
=== Bucketed Search Running Benchmarks (100000000 queries) ===
Bucketed Search Querry Time: 10066.177 ms
=== Results ===
=== Results for Binary Search ===
Querry Binary Search:
  Total Time:   51306.989 ms
  Per Query:    0.000513 ms
Querry + Prepare Binary Search:
  Total Time:    62240.724 ms
  Per Query:     0.000622 ms
=== Results for Bucketed Search ===
Querry Bucketed Search:
  Total Time:   10066.177 ms
  Per Query:    0.000101 ms
Querry + Prepare Bucketed Search:
  Total Time:    15834.229 ms
  Per Query:     0.000158 ms

=== Performance Leaderboard & Gaps ===
#1  | Bucketed Search                | Query Time:  10066.177 ms | Total Time:  15834.229 ms
     └─ vs #2: FASTER by 5.10x (saved 41240.812 ms)
     └─ Dominates from query #1 (better build time & query time)
#2  | Binary Search                  | Query Time:  51306.989 ms | Total Time:  62240.724 ms

SUCCESS: Checksums match perfectly!
Binary Search Checksum: 5000217653486814
Bucketed Search Checksum: 5000217653486814
=== Binary Search Info ===
=== Bucketed Search Info ===

=== Learned Index Parameters (O(1) Piecewise Linear) ===
  Dataset Size:        100000000
  Min / Max Value:     -19999999.8401 / 19999999.9750
  Range Span:          39999999.8152
  Segments Count:      65536
  Total Buckets:       100000001
===============================================


=== Index Quality & Query Cost Analysis ===
  Total Buckets:       100000000
  Non-empty Buckets:   63223555 (63.22%)
  Empty Buckets:       36776445
  Avg Elements/Bucket: 1.58
  Max Bucket Size:     12 elements
  Max Elements Scanned per Query (Worst Case ±1 window): ~36
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
            -19999999.84                          0.07                           19999999.98

=== Step Distribution Enviroment ===
=== Prepare Data

Generating Data

Generating Skewed Data with Outliers
[====================================>   ] [00m:01s<00m:00s]                                                                                                                                                                                
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
Binary Search Prepare Time: 21068.013 ms
=== Binary Search Running Benchmarks (100000000 queries) ===
Binary Search Querry Time: 52472.230 ms
=== Bucketed Search Setting Data ===
=== Bucketed Search Prepare Structure ===
Bucketed Search Prepare Time: 5585.012 ms
=== Bucketed Search Running Benchmarks (100000000 queries) ===
Bucketed Search Querry Time: 9916.422 ms
=== Results ===
=== Results for Binary Search ===
Querry Binary Search:
  Total Time:   52472.230 ms
  Per Query:    0.000525 ms
Querry + Prepare Binary Search:
  Total Time:    73540.243 ms
  Per Query:     0.000735 ms
=== Results for Bucketed Search ===
Querry Bucketed Search:
  Total Time:   9916.422 ms
  Per Query:    0.000099 ms
Querry + Prepare Bucketed Search:
  Total Time:    15501.434 ms
  Per Query:     0.000155 ms

=== Performance Leaderboard & Gaps ===
#1  | Bucketed Search                | Query Time:   9916.422 ms | Total Time:  15501.434 ms
     └─ vs #2: FASTER by 5.29x (saved 42555.809 ms)
     └─ Dominates from query #1 (better build time & query time)
#2  | Binary Search                  | Query Time:  52472.230 ms | Total Time:  73540.243 ms

SUCCESS: Checksums match perfectly!
Binary Search Checksum: 4999964242607086
Bucketed Search Checksum: 4999964242607086
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
  Non-empty Buckets:   63212686 (63.21%)
  Empty Buckets:       36787314
  Avg Elements/Bucket: 1.58
  Max Bucket Size:     10 elements
  Max Elements Scanned per Query (Worst Case ±1 window): ~30
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

