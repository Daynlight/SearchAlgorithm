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



## Test on 1B data 7.5GB in Release mode linear distribution 50000000 searches
```cpp
#define DATA_SIZE 1000000000 
#define DATA_MIN_VAL -20000000
#define DATA_MAX_VAL 20000000
#define NUM_QUERIES 50000000
#define BUCKET_SIZE 100000000
```
```bash
========================
=== Seach Algorithms ===
========================
=== Prepare Data
=== Loading Data from File ===
Successfully loaded size: 1000000000

size: 1000000000
size: 1000000000
=== Building Bucket Index Structure ===
Index Build Time: 15587.252 ms
Generating Queries for Standard Case
=== Running Benchmarks (50000000 queries) ===
Standard Binary Search:
  Total Time:   61288.494 ms
  Per Query:    0.001226 ms
Bucketed Binary Search:
  Total Time:   16683.708 ms
  Per Query:    0.000334 ms

>>> Bucketed Binary Search is FASTER by 3.67x! <<<
Total Standard Binary Search:
  Total Time:    61288.494 ms
  Per Query:     0.001226 ms
Total Bucketed Binary Search (Build + Queries):
  Total Time:    32270.960 ms
  Per Query:     0.000645 ms

>>> Total Bucketed Binary Search is FASTER by 1.90x! <<<
SUCCESS: Checksums match perfectly!

=== Learned Index Parameters (Flat Direct) ===
  Dataset Size:       1000000000
  Min / Max Value:    -19999999.9574 / 19999999.9571
  Range Span:         39999999.9146
  Softsign Params:    param_a = 0.0000001276, param_b = 70.4979
  Global Model:       slope = 139200120.8295, intersect = -19600010.7824
  Total Buckets:      100000000
===============================================
```



## Test on 1B data 7.5GB in Release mode skewed and outline distribution 50000000 searches
```cpp
#define DATA_SIZE 1000000000 

#define DATA_DENSE_MIN_VAL -10
#define DATA_DENSE_MAX_VAL 10
#define DATA_OFFSET_MIN_VAL 1000000
#define DATA_OFFSET_MAX_VAL 10000000
#define DATA_OFFSET_SIZE 100

#define NUM_QUERIES 50000000
#define BUCKET_SIZE 100000000
```
```bash
========================
=== Seach Algorithms ===
========================
=== Prepare Data
=== Loading Data from File ===
Successfully loaded size: 1000000000

size: 1000000000
size: 1000000000
=== Building Bucket Index Structure ===
Index Build Time: 14922.781 ms
Generating Queries for Offset Case
=== Running Benchmarks (50000000 queries) ===
Standard Binary Search:
  Total Time:   56592.497 ms
  Per Query:    0.001132 ms
Bucketed Binary Search:
  Total Time:   15380.358 ms
  Per Query:    0.000308 ms

>>> Bucketed Binary Search is FASTER by 3.68x! <<<
Total Standard Binary Search:
  Total Time:    56592.497 ms
  Per Query:     0.001132 ms
Total Bucketed Binary Search (Build + Queries):
  Total Time:    30303.139 ms
  Per Query:     0.000606 ms

>>> Total Bucketed Binary Search is FASTER by 1.87x! <<<
SUCCESS: Checksums match perfectly!

=== Learned Index Parameters (Flat Direct) ===
  Dataset Size:       1000000000
  Min / Max Value:    -10.0000 / 9805299.4124
  Range Span:         9805309.4124
  Softsign Params:    param_a = 0.2550957623, param_b = 0.0003
  Global Model:       slope = 116387852.6585, intersect = -16387829.3929
  Total Buckets:      100000000
===============================================
```

