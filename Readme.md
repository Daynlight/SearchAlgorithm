## Installation
### VCPKG
[Link](https://vcpkg.io/en/)

### Download dependencies
```bash
vcpkg install --x-install-root=./vendor
```

### Compilation and Run
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build && nice -20 ./build/search
```



## Main Idea
Tworzymy dodatkową strukture $O(n)$ w której możemy znaleźć odpowiedni przedział danych w $O(1)$ potem w tym przedziale przeszukujemy po przez użycie Binary Searcha $O(log_{2}{n})$. Duża zależność od rozkładu danych w najgorszym przypadku $O(log_{2}{n})$.



## Organiczenia
Struktura jest mocna w masowym przeszukiwaniu nie edytowalnych danych ale dla dynamicznie zmieniających się bedzie słabasza przez powód generowania dodatkowej tablicy reversu. Można by było rozbudować rozwiazanie o insert, remove dla edytowania tylko zmiany i aktualizacjia bucketow. Rozkład bucketow jest liniowy co powoduje edge casy jak np 1M 0.1f i jeden 2000000.0f wtedy do jednego bucketu trafia masa danych i przeszukujemy dużo danych z jednego bucketa w $O(log_{2}{n})$ worst case scenario. Można by było użyć metoda statystycznych by generować buckety o różnych rozmiarach oraz uzyć Statystyki Normalnej lub innej odwzorowującej nasze dane.



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
Successfully loaded size: 100000000

size: 100000000

size: 100000000
=== Prepare Testing Data
Generating Queries for Standard Case

=== Binary Search Setting Data ===
=== Binary Search Prepare Structure ===
Binary Search Prepare Time: 0.000 ms
=== Binary Search Running Benchmarks (100000000 queries) ===
Binary Search Querry Time: 131015.862 ms
=== Bucketed Search Setting Data ===
=== Bucketed Search Prepare Structure ===
Bucketed Search Prepare Time: 2967.251 ms
=== Bucketed Search Running Benchmarks (100000000 queries) ===
Bucketed Search Querry Time: 34298.032 ms
=== Results ===
=== Results for Binary Search ===
Querry Binary Search:
  Total Time:   131015.862 ms
  Per Query:    0.001310 ms
Querry + Prepare Binary Search:
  Total Time:    131015.862 ms
  Per Query:     0.001310 ms
=== Results for Bucketed Search ===
Querry Bucketed Search:
  Total Time:   34298.032 ms
  Per Query:    0.000343 ms
Querry + Prepare Bucketed Search:
  Total Time:    37265.282 ms
  Per Query:     0.000373 ms

=== Performance Leaderboard & Gaps ===
#1  | Bucketed Search                | Query Time:  34298.032 ms | Total Time:  37265.282 ms
     └─ vs #2: FASTER by 3.82x (saved 96717.831 ms)
     └─ Min Queries for Profit: 3067946 queries (3.07% of dataset)
#2  | Binary Search                  | Query Time: 131015.862 ms | Total Time: 131015.862 ms

SUCCESS: Checksums match perfectly!
Binary Search Checksum: 4999536060897242
Bucketed Search Checksum: 4999536060897242
=== Binary Search Info ===
=== Bucketed Search Info ===

=== Learned Index Parameters (O(1) Piecewise Linear) ===
  Dataset Size:        100000000
  Min / Max Value:     -19999999.7405 / 19999999.7634
  Range Span:          39999999.5039
  Segments Count:      65536
  Total Buckets:       100000000
===============================================


=== Index Quality & Query Cost Analysis ===
  Total Buckets:       100000000
  Non-empty Buckets:   63226894 (63.23%)
  Empty Buckets:       36773106
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
            -19999999.74                          0.01                           19999999.76

=== Step Distribution Enviroment ===
=== Prepare Data
=== Loading Data from File ===
Successfully loaded size: 100000000

size: 100000000

size: 100000000
=== Prepare Testing Data
Generating Queries for Offset Case

=== Binary Search Setting Data ===
=== Binary Search Prepare Structure ===
Binary Search Prepare Time: 0.000 ms
=== Binary Search Running Benchmarks (100000000 queries) ===
Binary Search Querry Time: 98710.346 ms
=== Bucketed Search Setting Data ===
=== Bucketed Search Prepare Structure ===
Bucketed Search Prepare Time: 2396.850 ms
=== Bucketed Search Running Benchmarks (100000000 queries) ===
Bucketed Search Querry Time: 28920.375 ms
=== Results ===
=== Results for Binary Search ===
Querry Binary Search:
  Total Time:   98710.346 ms
  Per Query:    0.000987 ms
Querry + Prepare Binary Search:
  Total Time:    98710.346 ms
  Per Query:     0.000987 ms
=== Results for Bucketed Search ===
Querry Bucketed Search:
  Total Time:   28920.375 ms
  Per Query:    0.000289 ms
Querry + Prepare Bucketed Search:
  Total Time:    31317.226 ms
  Per Query:     0.000313 ms

=== Performance Leaderboard & Gaps ===
#1  | Bucketed Search                | Query Time:  28920.375 ms | Total Time:  31317.226 ms
     └─ vs #2: FASTER by 3.41x (saved 69789.970 ms)
     └─ Min Queries for Profit: 3434376 queries (3.43% of dataset)
#2  | Binary Search                  | Query Time:  98710.346 ms | Total Time:  98710.346 ms

SUCCESS: Checksums match perfectly!
Binary Search Checksum: 6337127427305815
Bucketed Search Checksum: 6337127427305815
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
  Non-empty Buckets:   63220060 (63.22%)
  Empty Buckets:       36779940
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

