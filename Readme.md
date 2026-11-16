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

