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



## Tetowane na 1B danych w Release mode
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
Index Build Time: 9116.048 ms
=== Running Benchmarks (50000000 queries) ===
Standard Binary Search:
  Total Time:   75721.960 ms
  Per Query:    0.001514 ms
Bucketed Binary Search:
  Total Time:   10255.477 ms
  Per Query:    0.000205 ms

>>> Bucketed Binary Search is FASTER by 7.38x! <<<
```
