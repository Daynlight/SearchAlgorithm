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
### Build index
1. We split `data.size()` into `n` equal sections. For each section we calulate **slope** and **intersect** base on `min` and `max` value in range. We also store `min` and `max` of all data accessing each segment is geting percent from value relative to global `min` and `max`. This structure is used to model bucket distribution function that optimzes buckets. Function is monotonic bijection for each entry we get one bucket.
2. For each element in `data` we calulate `bid = bucket_clasificator(value)` and emplace index in corect bucket.

### Search for value
1. **Explenation**: When we search for value we first calulate `bid = bucket_clasificator(value)`. Then we get left and right neighbor. Check `min` and `max` of each one and chose one to search in by BSearch. 
2. **Time Complexity**: Becouse of `buckets == data.size()` we get $O(1) + O(log_{2}{\frac{n}{b}}) \implies O(1)$



<!-- ## Organiczenia
# Struktura jest mocna w masowym przeszukiwaniu nie edytowalnych danych ale dla dynamicznie zmieniających się bedzie słabasza przez powód generowania dodatkowej tablicy reversu. Można by było rozbudować rozwiazanie o insert, remove dla edytowania tylko zmiany i aktualizacjia bucketow. Rozkład bucketow jest liniowy co powoduje edge casy jak np 1M 0.1f i jeden 2000000.0f wtedy do jednego bucketu trafia masa danych i przeszukujemy dużo danych z jednego bucketa w $O(log_{2}{n})$ worst case scenario. Można by było użyć metoda statystycznych by generować buckety o różnych rozmiarach oraz uzyć Statystyki Normalnej lub innej odwzorowującej nasze dane.
-->


## Results 
```txt
========================
=== Seach Algorithms ===
========================
=== Creating Benchmark Envitoments ===
=== Creating Benchmark Structure ===
=== Linear Distribution Enviroment ===
=== Prepare Data
=== Loading Data from File ===
Successfully loaded size: 10000000

size: 10000000

size: 10000000
=== Prepare Testing Data
Generating Queries for Standard Case

=== Binary Search Setting Data ===
=== Binary Search Prepare Structure ===

Sorting Data
Binary Search Prepare Time: 1995.522 ms
=== Binary Search Running Benchmarks (10000000 queries) ===
Binary Search Querry Time: 6952.142 ms
=== Bucketed Search Setting Data ===
=== Bucketed Search Prepare Structure ===
Bucketed Search Prepare Time: 890.198 ms
=== Bucketed Search Running Benchmarks (10000000 queries) ===
Bucketed Search Querry Time: 2383.042 ms
=== Results ===
=== Results for Binary Search ===
Querry Binary Search:
  Total Time:   6952.142 ms
  Per Query:    0.000695 ms
Querry + Prepare Binary Search:
  Total Time:    8947.664 ms
  Per Query:     0.000895 ms
=== Results for Bucketed Search ===
Querry Bucketed Search:
  Total Time:   2383.042 ms
  Per Query:    0.000238 ms
Querry + Prepare Bucketed Search:
  Total Time:    3273.240 ms
  Per Query:     0.000327 ms

=== Performance Leaderboard & Gaps ===
#1  | Bucketed Search                | Query Time:   2383.042 ms | Total Time:   3273.240 ms
     └─ vs #2: FASTER by 2.92x (saved 4569.100 ms)
     └─ Dominates from query #1 (better build time & query time)
#2  | Binary Search                  | Query Time:   6952.142 ms | Total Time:   8947.664 ms

SUCCESS: Checksums match perfectly!
Binary Search Checksum: 50002218958704
Bucketed Search Checksum: 50002218958704
=== Binary Search Info ===
=== Bucketed Search Info ===

=== Learned Index Parameters (O(1) Piecewise Linear) ===
  Dataset Size:        10000000
  Min / Max Value:     -19999998.8771 / 19999999.5570
  Range Span:          39999998.4341
  Segments Count:      65536
  Total Buckets:       10000000
===============================================


=== Index Quality & Query Cost Analysis ===
  Total Buckets:       10000000
  Non-empty Buckets:   6333573 (63.34%)
  Empty Buckets:       3666427
  Avg Elements/Bucket: 1.58
  Max Bucket Size:     11 elements
  Max Elements Scanned per Query (Worst Case ±1 window): ~33
===========================================


=== Classifier Graph: bucket_clasificator(x) [Segments: 65536] ===
   9999999 |                                                                              **
   9565216 |                                                                          ****  
   9130434 |                                                                       ***      
   8695651 |                                                                   ****         
   8260869 |                                                                ***             
   7826086 |                                                             ***                
   7391304 |                                                         ****                   
   6956521 |                                                      ***                       
   6521738 |                                                  ****                          
   6086956 |                                               ***                              
   5652173 |                                           ****                                 
   5217391 |                                        ***                                     
   4782608 |                                     ***                                        
   4347826 |                                 ****                                           
   3913043 |                              ***                                               
   3478261 |                          ****                                                  
   3043478 |                       ***                                                      
   2608695 |                   ****                                                         
   2173913 |                ***                                                             
   1739130 |             ***                                                                
   1304348 |         ****                                                                   
    869565 |      ***                                                                       
    434783 |  ****                                                                          
         0 |**                                                                              
           +--------------------------------------------------------------------------------
            -19999998.88                          0.34                           19999999.56

=== Step Distribution Enviroment ===
=== Prepare Data
=== Loading Data from File ===
Successfully loaded size: 10000000

size: 10000000

size: 10000000
=== Prepare Testing Data
Generating Queries for Offset Case

Generating Skewed Data with Outliers

=== Binary Search Setting Data ===
=== Binary Search Prepare Structure ===

Sorting Data
Binary Search Prepare Time: 1832.717 ms
=== Binary Search Running Benchmarks (10000000 queries) ===
Binary Search Querry Time: 5739.826 ms
=== Bucketed Search Setting Data ===
=== Bucketed Search Prepare Structure ===
Bucketed Search Prepare Time: 738.960 ms
=== Bucketed Search Running Benchmarks (10000000 queries) ===
Bucketed Search Querry Time: 2058.743 ms
=== Results ===
=== Results for Binary Search ===
Querry Binary Search:
  Total Time:   5739.826 ms
  Per Query:    0.000574 ms
Querry + Prepare Binary Search:
  Total Time:    7572.543 ms
  Per Query:     0.000757 ms
=== Results for Bucketed Search ===
Querry Bucketed Search:
  Total Time:   2058.743 ms
  Per Query:    0.000206 ms
Querry + Prepare Bucketed Search:
  Total Time:    2797.702 ms
  Per Query:     0.000280 ms

=== Performance Leaderboard & Gaps ===
#1  | Bucketed Search                | Query Time:   2058.743 ms | Total Time:   2797.702 ms
     └─ vs #2: FASTER by 2.79x (saved 3681.083 ms)
     └─ Dominates from query #1 (better build time & query time)
#2  | Binary Search                  | Query Time:   5739.826 ms | Total Time:   7572.543 ms

SUCCESS: Checksums match perfectly!
Binary Search Checksum: 49998976501085
Bucketed Search Checksum: 49998976501085
=== Binary Search Info ===
=== Bucketed Search Info ===

=== Learned Index Parameters (O(1) Piecewise Linear) ===
  Dataset Size:        10000000
  Min / Max Value:     -10.0000 / 310.0000
  Range Span:          320.0000
  Segments Count:      65536
  Total Buckets:       10000000
===============================================


=== Index Quality & Query Cost Analysis ===
  Total Buckets:       10000000
  Non-empty Buckets:   6325252 (63.25%)
  Empty Buckets:       3674748
  Avg Elements/Bucket: 1.58
  Max Bucket Size:     10 elements
  Max Elements Scanned per Query (Worst Case ±1 window): ~30
===========================================


=== Classifier Graph: bucket_clasificator(x) [Segments: 65536] ===
   9999999 |                                                                               *
   9565216 |                                                                              * 
   9130434 |                                                                             *  
   8695651 |                                                                            *   
   8260869 |                                                                            |   
   7826086 |                                                                           *    
   7391304 |                                                      *********************     
   6956521 |                                                     *                          
   6521738 |                                                    *                           
   6086956 |                                                    |                           
   5652173 |                                                   *                            
   5217391 |                              *********************                             
   4782608 |                             *                                                  
   4347826 |                            *                                                   
   3913043 |                            |                                                   
   3478261 |                           *                                                    
   3043478 |                          *                                                     
   2608695 |     *********************                                                      
   2173913 |    *                                                                           
   1739130 |    |                                                                           
   1304348 |   *                                                                            
    869565 |  *                                                                             
    434783 | *                                                                              
         0 |*                                                                               
           +--------------------------------------------------------------------------------
            -10.00                               150.00                               310.00

```

