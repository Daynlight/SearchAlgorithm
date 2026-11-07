#include <random>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <map>
#include <chrono>

#include <fmt/base.h>
#include <fmt/color.h>
#include <fmt/ranges.h>
#include <indicators/progress_bar.hpp>



#define DATA_SIZE 1000000000 
#define DATA_MIN_VAL -20000000
#define DATA_MAX_VAL 20000000

#define DATA_DENSE_MIN_VAL -10
#define DATA_DENSE_MAX_VAL 10
#define DATA_OFFSET_MIN_VAL 1000000
#define DATA_OFFSET_MAX_VAL 10000000
#define DATA_OFFSET_SIZE 100

#define OFFSET_CASE true 
#define NUM_QUERIES 50000000
#define BUCKET_SIZE 100000000



struct BucketIndex {
  double min_val{0};
  double max_val{0};
  double range_span{1.0};
  double param_b{0.0};
  double param_a{1.0};
  double slope{1.0};
  double intersect{0.0};

  std::vector<long long> bucket_start;
  std::vector<long long> bucket_end;
};



struct SortedDataset {
  std::vector<double> data;
  BucketIndex bucket_index;



  // O(1)
  inline double transform_val(double val) const {
    double dx = (val - bucket_index.param_b) * bucket_index.param_a;
    return 0.5 * (dx / (1.0 + std::abs(dx)) + 1.0);
  };



  // O(1)
  inline void calculate_parameters(long long n, long long num_buckets) {
    const double* d = data.data();
    double min_v = d[0];
    double max_v = d[n - 1];
    
    double p1  = d[static_cast<long long>(n * 0.01)];
    double p50 = d[n >> 1];
    double p99 = d[static_cast<long long>(n * 0.99)];
    
    bucket_index.param_b = p50; 

    double dense_spread = std::max(p99 - p50, p50 - p1);
    if (__builtin_expect(dense_spread <= 0.0, 0)) dense_spread = bucket_index.range_span;
    if (dense_spread <= 0.0) dense_spread = 1.0;
    
    bucket_index.param_a = 2.5 / dense_spread;

    double tx_min = transform_val(min_v);
    double tx_max = transform_val(max_v);
    double dx = tx_max - tx_min;
    
    if (std::abs(dx) > 1e-9) {
      bucket_index.slope = static_cast<double>(num_buckets) / dx;
      bucket_index.intersect = -bucket_index.slope * tx_min;
    } else {
      bucket_index.slope = num_buckets;
      bucket_index.intersect = 0.0;
    };
  };



  // O(1) 
  long long bucket_clasificator(double val) const {
    double tx = transform_val(val);
    double estimated_bucket = bucket_index.slope * tx + bucket_index.intersect;
    return static_cast<long long>(estimated_bucket);
  };



  // O(n)
  void build_index(long long num_buckets = 100000000) {      
    if (data.empty()) return;
    long long n = data.size();

    bucket_index.min_val = data.front();
    bucket_index.max_val = data.back();
    bucket_index.range_span = bucket_index.max_val - bucket_index.min_val;
    if (bucket_index.range_span <= 0) bucket_index.range_span = 1.0;

    calculate_parameters(n, num_buckets);

    bucket_index.bucket_start.assign(num_buckets, -1);
    bucket_index.bucket_end.assign(num_buckets, -1);

    const double* d_ptr = data.data();

    for (long long i = 0; i < n; ++i) {
      long long b_id = bucket_clasificator(d_ptr[i]);
      b_id = std::clamp(b_id, 0LL, num_buckets - 1);
      if (bucket_index.bucket_start[b_id] == -1) bucket_index.bucket_start[b_id] = i;
      bucket_index.bucket_end[b_id] = i;
    };

    long long last_start = 0;
    long long last_end = n - 1;
    
    for (long long b = 0; b < num_buckets; ++b) {
      if (bucket_index.bucket_start[b] != -1) {
        last_start = bucket_index.bucket_start[b];
        last_end = bucket_index.bucket_end[b];
        break;
      };
    };

    for (long long b = 0; b < num_buckets; ++b) {
      if (bucket_index.bucket_start[b] != -1) {
        last_start = bucket_index.bucket_start[b];
        last_end = bucket_index.bucket_end[b];
      } else {
        bucket_index.bucket_start[b] = last_start;
        bucket_index.bucket_end[b] = last_end;
      };
    };

    long long max_recorded_end = bucket_index.bucket_end[num_buckets - 1];
    for (long long b = num_buckets - 1; b >= 0; --b) {
      if (bucket_index.bucket_end[b] == max_recorded_end) bucket_index.bucket_end[b] = n - 1;
      else break;
    };
  };



  // O(log2(n))
  long long find_nearest(double target) const {  
    if (data.empty()) return -1;
    long long left = 0;
    long long right = static_cast<long long>(data.size()) - 1;

    if (target <= data[left]) return left;
    if (target >= data[right]) return right;

    while (left <= right) {
      long long mid = left + (right - left) / 2;
      if (data[mid] == target) return mid;
      else if (data[mid] < target) left = mid + 1;
      else right = mid - 1;
    };

    if (right < 0) return 0;
    if (left >= static_cast<long long>(data.size())) return static_cast<long long>(data.size()) - 1;

    if (std::abs(data[left] - target) < std::abs(data[right] - target)) return left;
    return right;
  };



  // O(1) + O(log2(k))
  long long find_nearest_bucketed(double target) const {
    const long long n = data.size();
    if (n == 0) return -1;

    const double* d_ptr = data.data();
    if (target <= d_ptr[0]) return 0;
    if (target >= d_ptr[n - 1]) return n - 1;

    long long num_buckets = static_cast<long long>(bucket_index.bucket_start.size());
    long long b_id = bucket_clasificator(target);
    b_id = (b_id < 0) ? 0 : (b_id >= num_buckets ? num_buckets - 1 : b_id);

    const int ERROR_MARGIN = 1;
    long long min_b = (b_id > ERROR_MARGIN) ? (b_id - ERROR_MARGIN) : 0;
    long long max_b = (b_id + ERROR_MARGIN < num_buckets) ? (b_id + ERROR_MARGIN) : (num_buckets - 1);

    long long left = bucket_index.bucket_start[min_b];
    long long right = bucket_index.bucket_end[max_b];

    if (max_b >= num_buckets - 2) right = n - 1;

    while (left <= right) {
      long long mid = left + ((right - left) >> 1);
      double val = d_ptr[mid];
      if (val == target) return mid;
      if (val < target) left = mid + 1;
      else right = mid - 1;
    };

    if (right < bucket_index.bucket_start[min_b]) return bucket_index.bucket_start[min_b];
    if (left >= n) return n - 1;

    return (std::abs(d_ptr[left] - target) < std::abs(d_ptr[right] - target)) ? left : right;
  };



  void print_params() const {
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold, "\n=== Learned Index Parameters (Flat Direct) ===\n");
    fmt::print(fmt::fg(fmt::color::green),  "  Dataset Size:       {}\n", data.size());
    fmt::print(fmt::fg(fmt::color::yellow), "  Min / Max Value:    {:.4f} / {:.4f}\n", bucket_index.min_val, bucket_index.max_val);
    fmt::print(fmt::fg(fmt::color::yellow), "  Range Span:         {:.4f}\n", bucket_index.range_span);
    fmt::print(fmt::fg(fmt::color::yellow), "  Softsign Params:    param_a = {:.10f}, param_b = {:.4f}\n", bucket_index.param_a, bucket_index.param_b);
    fmt::print(fmt::fg(fmt::color::green) | fmt::emphasis::bold, "  Global Model:       slope = {:.4f}, intersect = {:.4f}\n", bucket_index.slope, bucket_index.intersect);
    fmt::print(fmt::fg(fmt::color::magenta), "  Total Buckets:      {}\n", bucket_index.bucket_start.size());
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold, "===============================================\n\n");
  };
};


int main(){
  fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "========================");
  fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "=== Seach Algorithms ===");
  fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "========================");
  


  fmt::println(fg(fmt::color::yellow), "=== Prepare Data");
  std::string filename = "data.bin";
  if(OFFSET_CASE) filename = "offset.data.bin";
  std::vector<double> data;
  data.reserve(DATA_SIZE);

  if (std::filesystem::exists(filename)) {
    fmt::println(fg(fmt::color::pink), "=== Loading Data from File ===");
    std::ifstream file(filename, std::ios::binary);
    if (file) {
      size_t size = 0;
      file.read(reinterpret_cast<char*>(&size), sizeof(size));
      data.resize(size);
      file.read(reinterpret_cast<char*>(data.data()), size * sizeof(double));
      fmt::println(fg(fmt::color::green), "Successfully loaded size: {}", data.size());
    } else fmt::println(fg(fmt::color::red), "Error: Could not open file for reading.");
  } else 
  {
    indicators::ProgressBar bar{
      indicators::option::BarWidth{40},
      indicators::option::Start{"["},
      indicators::option::End{"]"},
      indicators::option::Fill{"="},
      indicators::option::Lead{">"},
      indicators::option::Remainder{" "},
      indicators::option::MaxProgress{DATA_SIZE},
      indicators::option::ShowElapsedTime{true},
      indicators::option::ShowRemainingTime{true},
      indicators::option::ForegroundColor{indicators::Color::green}
    };
  
    fmt::println(fg(fmt::color::pink), "\nGenerating Data");
    if(OFFSET_CASE){
      std::random_device dev;
      std::mt19937 rng(dev());
      std::uniform_real_distribution<double> dist_dense(DATA_DENSE_MIN_VAL, DATA_DENSE_MAX_VAL);
      std::uniform_real_distribution<double> dist_outlier(DATA_OFFSET_MIN_VAL, DATA_OFFSET_MAX_VAL);
      
      fmt::println(fg(fmt::color::pink), "\nGenerating Skewed Data with Outliers");
      for(long long i = 0; i < DATA_SIZE; i++) {
        if (i < DATA_SIZE - DATA_OFFSET_SIZE) data.emplace_back(dist_dense(rng));
        else data.emplace_back(dist_outlier(rng));
        if (i % 10000000 == 0) bar.set_progress(i);
      };
      fmt::print("\n");
      fmt::println(fg(fmt::color::pink), "\nSorting Data");
      std::sort(data.begin(), data.end());
    }
    else {
      std::random_device dev;
      std::mt19937 rng(dev());
      std::uniform_real_distribution<double> dist(DATA_MIN_VAL, DATA_MAX_VAL);
     
      for(long long i = 0; i < DATA_SIZE; i++) {
        data.emplace_back(dist(rng));
        
        if (i % 10000 == 0) bar.set_progress(i);
      };
      fmt::print("\n");

      fmt::println(fg(fmt::color::pink), "\nSorting Data");
      std::sort(data.begin(), data.end());
    };


    fmt::println(fg(fmt::color::yellow), "=== Saving Data to Binary File ===");
    std::ofstream file(filename, std::ios::binary);
    if (file) {
      size_t size = data.size();
      file.write(reinterpret_cast<const char*>(&size), sizeof(size));
      file.write(reinterpret_cast<const char*>(data.data()), size * sizeof(double));
      fmt::println(fg(fmt::color::green), "Data saved to {}", filename);
    } else fmt::println(fg(fmt::color::red), "Error: Could not open file for writing.");
  };

  fmt::println(fg(fmt::color::pink), "\nsize: {}", data.size());



  SortedDataset dataset;
  dataset.data = std::move(data);
  fmt::println(fg(fmt::color::pink), "size: {}", dataset.data.size());

  fmt::println(fg(fmt::color::yellow), "=== Building Bucket Index Structure ===");
  auto start_build = std::chrono::high_resolution_clock::now();
  dataset.build_index(BUCKET_SIZE);
  auto end_build = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration_build = end_build - start_build;
  fmt::println(fg(fmt::color::green), "Index Build Time: {:.3f} ms", duration_build.count());  std::vector<double> test_targets;
  
  test_targets.reserve(NUM_QUERIES);
  {
    std::random_device dev;
    std::mt19937 rng(dev());

    if (OFFSET_CASE) {
      fmt::println(fg(fmt::color::pink), "Generating Queries for Offset Case");
      std::uniform_real_distribution<double> dist_dense(DATA_DENSE_MIN_VAL, DATA_DENSE_MAX_VAL);
      std::uniform_real_distribution<double> dist_outlier(DATA_OFFSET_MIN_VAL, DATA_OFFSET_MAX_VAL);
      std::bernoulli_distribution choice(0.99); 
      
      for(int i = 0; i < NUM_QUERIES; ++i) {
        if (choice(rng)) test_targets.push_back(dist_dense(rng));
        else test_targets.push_back(dist_outlier(rng));
      };
    } 
    else {
      fmt::println(fg(fmt::color::pink), "Generating Queries for Standard Case");
      std::uniform_real_distribution<double> dist(DATA_MIN_VAL, DATA_MAX_VAL);
      for(int i = 0; i < NUM_QUERIES; ++i) test_targets.push_back(dist(rng));
    };
  };

  fmt::println(fg(fmt::color::yellow), "=== Running Benchmarks ({} queries) ===", NUM_QUERIES);

  auto start_std = std::chrono::high_resolution_clock::now();
  long long checksum_std = 0;
  for(int i = 0; i < NUM_QUERIES; ++i) {
    checksum_std += dataset.find_nearest(test_targets[i]);
  }
  auto end_std = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration_std = end_std - start_std;

  auto start_bucket = std::chrono::high_resolution_clock::now();
  long long checksum_bucket = 0;
  for(int i = 0; i < NUM_QUERIES; ++i) {
    checksum_bucket += dataset.find_nearest_bucketed(test_targets[i]);
  }
  auto end_bucket = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration_bucket = end_bucket - start_bucket;



  bool bucketed_is_faster = duration_bucket.count() < duration_std.count();
  auto bucket_color = bucketed_is_faster ? fg(fmt::color::green) : fg(fmt::color::red);

  fmt::println(fg(fmt::color::cyan), "Standard Binary Search:");
  fmt::println(fg(fmt::color::green), "  Total Time:   {:.3f} ms", duration_std.count());
  fmt::println(fg(fmt::color::green), "  Per Query:    {:.6f} ms", duration_std.count() / NUM_QUERIES);

  fmt::println(fg(fmt::color::cyan), "Bucketed Binary Search:");
  fmt::println(bucket_color, "  Total Time:   {:.3f} ms", duration_bucket.count());
  fmt::println(bucket_color, "  Per Query:    {:.6f} ms", duration_bucket.count() / NUM_QUERIES);
  
  if (bucketed_is_faster) {
    double speedup = duration_std.count() / duration_bucket.count();
    fmt::println(fg(fmt::color::green) | fmt::emphasis::bold, "\n>>> Bucketed Binary Search is FASTER by {:.2f}x! <<<", speedup);
  } else {
    double slowdown = duration_bucket.count() / duration_std.count();
    fmt::println(fg(fmt::color::red) | fmt::emphasis::bold, "\n>>> Bucketed Binary Search is SLOWER by {:.2f}x. <<<", slowdown);
  };



  double total_time_bucketed = duration_build.count() + duration_bucket.count();
  bool total_bucketed_is_faster = total_time_bucketed < duration_std.count();
  auto total_bucket_color = total_bucketed_is_faster ? fg(fmt::color::green) : fg(fmt::color::red);
  
  fmt::println(fg(fmt::color::cyan), "Total Standard Binary Search:");
  fmt::println(fg(fmt::color::green), "  Total Time:    {:.3f} ms", duration_std.count());
  fmt::println(fg(fmt::color::green), "  Per Query:     {:.6f} ms", duration_std.count() / NUM_QUERIES);

  fmt::println(fg(fmt::color::cyan), "Total Bucketed Binary Search (Build + Queries):");
  fmt::println(total_bucket_color, "  Total Time:    {:.3f} ms", total_time_bucketed);
  fmt::println(total_bucket_color, "  Per Query:     {:.6f} ms", total_time_bucketed / NUM_QUERIES);
  
  if (total_bucketed_is_faster) {
    double speedup = duration_std.count() / total_time_bucketed;
    fmt::println(fg(fmt::color::green) | fmt::emphasis::bold, "\n>>> Total Bucketed Binary Search is FASTER by {:.2f}x! <<<", speedup);
  } else {
    double slowdown = total_time_bucketed / duration_std.count();
    fmt::println(fg(fmt::color::red) | fmt::emphasis::bold, "\n>>> Total Bucketed Binary Search is SLOWER by {:.2f}x. <<<", slowdown);
  };
  
  if (checksum_std != checksum_bucket) {
    fmt::println(fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: Checksums do not match! Bucketed search returned wrong results!");
    fmt::println(fg(fmt::color::red), "Standard Checksum: {}, Bucketed Checksum: {}", checksum_std, checksum_bucket);
  } 
  else fmt::println(fg(fmt::color::green), "SUCCESS: Checksums match perfectly!");


  
  dataset.print_params();



  if (checksum_std == -999999) fmt::print("");
  if (checksum_bucket == -999999) fmt::print("");

  return 0;
};

