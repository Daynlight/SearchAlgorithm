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



#define DATA_SIZE 100000000 
#define DATA_MIN_VAL -20000000
#define DATA_MAX_VAL 20000000

#define DATA_DENSE_MIN_VAL -10
#define DATA_DENSE_MAX_VAL 10
#define DATA_OFFSET_MIN_VAL 1000000
#define DATA_OFFSET_MAX_VAL 10000000
#define DATA_OFFSET_SIZE 100

#define OFFSET_CASE false 
#define NUM_QUERIES 100000000 
#define BUCKET_SIZE 0 



struct BucketIndex {
  double min_val{0};
  double max_val{0};
  double range_span{1.0};
  double param_b{0.0};
  double param_a{1.0};
  double slope{1.0};
  double intersect{0.0};

  std::vector<long long> bucket_start;
  std::vector<long long> counts;
};



struct SortedDataset {
  std::vector<double> data;
  BucketIndex bucket_index;



  inline double fast_abs(double x) const {
    union {
      double d;
      uint64_t i;
    } u;
    u.d = x;
    u.i &= 0x7FFFFFFFFFFFFFFFULL;
    return u.d;
  };


  
  // O(1)
  inline double transform_val(double val) const {
    double dx = (val - bucket_index.param_b) * bucket_index.param_a;
    double denom = 1.0 + fast_abs(dx);
    return 0.5 * (dx / denom + 1.0);
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



#define CLASSIFY_FAST(val, slope, intersect, param_a, param_b, max_b_idx, b_out) \
  do { \
    double dx_macro_ = ((val) - (param_b)) * (param_a); \
    union { double d; uint64_t i; } u_macro_{dx_macro_}; \
    u_macro_.i &= 0x7FFFFFFFFFFFFFFFULL; \
    double tx_macro_ = 0.5 * ((dx_macro_ / (1.0 + u_macro_.d)) + 1.0); \
    long long b_macro_ = static_cast<long long>((slope) * tx_macro_ + (intersect)); \
    (b_out) = (b_macro_ < 0) ? 0 : (b_macro_ > (max_b_idx) ? (max_b_idx) : b_macro_); \
  } while(0)



  // O(n)
  void build_index(long long num_buckets = 0) {      
    if (data.empty()) return;
    long long n = data.size();
    if(num_buckets == 0) num_buckets = n;

    bucket_index.min_val = data.front();
    bucket_index.max_val = data.back();
    bucket_index.range_span = bucket_index.max_val - bucket_index.min_val;
    if (bucket_index.range_span <= 0) bucket_index.range_span = 1.0;

    calculate_parameters(n, num_buckets);

    bucket_index.bucket_start.resize(num_buckets);
    bucket_index.counts.assign(num_buckets, 0);

    const double* d_ptr = data.data();
    long long* start_ptr = bucket_index.bucket_start.data();
    long long* count_ptr = bucket_index.counts.data();
    
    const double slope = bucket_index.slope;
    const double intersect = bucket_index.intersect;
    const double param_a = bucket_index.param_a;
    const double param_b = bucket_index.param_b;
    const long long max_b_idx = num_buckets - 1;

    long long i = 0;
    for (; i <= n - 4; i += 4) {
      long long b0, b1, b2, b3;

      CLASSIFY_FAST(d_ptr[i],   slope, intersect, param_a, param_b, max_b_idx, b0);
      CLASSIFY_FAST(d_ptr[i+1], slope, intersect, param_a, param_b, max_b_idx, b1);
      CLASSIFY_FAST(d_ptr[i+2], slope, intersect, param_a, param_b, max_b_idx, b2);
      CLASSIFY_FAST(d_ptr[i+3], slope, intersect, param_a, param_b, max_b_idx, b3);

      count_ptr[b0]++;
      count_ptr[b1]++;
      count_ptr[b2]++;
      count_ptr[b3]++;
    };

    for (; i < n; ++i) {
      long long b_id;
      CLASSIFY_FAST(d_ptr[i], slope, intersect, param_a, param_b, max_b_idx, b_id);
      count_ptr[b_id]++;
    };

    long long current_pos = 0;
    for (long long b = 0; b < num_buckets; ++b) {
      long long cnt = count_ptr[b];
      if (cnt > 0) {
        start_ptr[b] = current_pos;
        current_pos += cnt;
      } 
      else start_ptr[b] = -1;
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
    long long max_b_idx = num_buckets - 1;

    long long b_id;
    CLASSIFY_FAST(target, bucket_index.slope, bucket_index.intersect, 
                          bucket_index.param_a, bucket_index.param_b, max_b_idx, b_id);

    long long min_b = (b_id > 0) ? (b_id - 1) : 0;
    long long max_b = (b_id < max_b_idx) ? (b_id + 1) : max_b_idx;

    long long left = -1;
    long long right = -1;

    bool found_bucket = false;
    
    for (long long b = min_b; b <= max_b; ++b) {
      long long cnt = bucket_index.counts[b];
      if (cnt == 0) continue;

      long long b_start = bucket_index.bucket_start[b];
      long long b_end = b_start + cnt - 1;

      if (target <= d_ptr[b_end]) {
        left = b_start;
        right = b_end;
        found_bucket = true;
        break;
      } else {
        left = b_start;
        right = b_end;
      };
    };

    if (!found_bucket && (left == -1 || right == -1)) return find_nearest(target);

    while (left <= right) {
      long long mid = left + ((right - left) >> 1);
      double val = d_ptr[mid];
      if (val == target) return mid;
      if (val < target) left = mid + 1;
      else right = mid - 1;
    };

    if (right < bucket_index.bucket_start[min_b]) return bucket_index.bucket_start[min_b];
    if (left >= n) return n - 1;

    double dist_left = fast_abs(d_ptr[left] - target);
    double dist_right = fast_abs(d_ptr[right] - target);
    
    return (dist_left < dist_right) ? left : right;
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



  void analyze_index_stats() const {
    if (bucket_index.counts.empty()) {
      fmt::println(fg(fmt::color::red), "Index is empty or not built!");
      return;
    };

    long long total_buckets = bucket_index.counts.size();
    long long non_empty_buckets = 0;
    long long max_bucket_size = 0;
    long long sum_bucket_size = 0;
    long long empty_buckets = 0;

    for (long long cnt : bucket_index.counts) {
      if (cnt > 0) {
        non_empty_buckets++;
        sum_bucket_size += cnt;
        if (cnt > max_bucket_size) {
          max_bucket_size = cnt;
        }
      } else {
        empty_buckets++;
      }
    }

    double avg_bucket_size = non_empty_buckets > 0 ? static_cast<double>(sum_bucket_size) / non_empty_buckets : 0.0;
    long long worst_case_elements_to_scan = max_bucket_size * 3;

    fmt::println(fg(fmt::color::yellow), "\n=== Index Quality & Query Cost Analysis ===");
    fmt::println("  Total Buckets:       {}", total_buckets);
    fmt::println("  Non-empty Buckets:   {} ({:.2f}%)", non_empty_buckets, (double)non_empty_buckets / total_buckets * 100.0);
    fmt::println("  Empty Buckets:       {}", empty_buckets);
    fmt::println("  Avg Elements/Bucket: {:.2f}", avg_bucket_size);
    fmt::println("  Max Bucket Size:     {} elements", max_bucket_size);
    fmt::println(fg(fmt::color::green), "  Max Elements Scanned per Query (Worst Case ±1 window): ~{}", worst_case_elements_to_scan);
    fmt::println("===========================================\n");
  }
};



std::vector<double> prepare_data(){
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
  } 
  else {
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

  return data;
};



std::vector<double> prepare_testing_data() {
  std::vector<double> test_targets;
  
  test_targets.reserve(NUM_QUERIES);
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

  return test_targets;
};



int main(){
  fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "========================");
  fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "=== Seach Algorithms ===");
  fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "========================");
  
  fmt::println(fg(fmt::color::yellow), "=== Prepare Data");
  std::vector<double> data = prepare_data();
  fmt::println("");

  fmt::println(fg(fmt::color::yellow), "=== Prepare Testing Data");
  std::vector<double> test_targets = prepare_testing_data();
  fmt::println("");


  SortedDataset dataset;
  dataset.data = std::move(data);
  fmt::println(fg(fmt::color::pink), "size: {}", dataset.data.size());

  fmt::println(fg(fmt::color::yellow), "=== Building Bucket Index Structure ===");
  auto start_build = std::chrono::high_resolution_clock::now();
  dataset.build_index(BUCKET_SIZE);
  auto end_build = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration_build = end_build - start_build;
  fmt::println(fg(fmt::color::green), "Index Build Time: {:.3f} ms", duration_build.count());  


  fmt::println(fg(fmt::color::yellow), "=== Running Benchmarks ({} queries) ===", NUM_QUERIES);

  auto start_std = std::chrono::high_resolution_clock::now();
  long long checksum_std = 0;
  for(int i = 0; i < NUM_QUERIES; ++i) checksum_std += dataset.find_nearest(test_targets[i]);
  auto end_std = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration_std = end_std - start_std;

  auto start_bucket = std::chrono::high_resolution_clock::now();
  long long checksum_bucket = 0;
  for(int i = 0; i < NUM_QUERIES; ++i) checksum_bucket += dataset.find_nearest_bucketed(test_targets[i]);
  auto end_bucket = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration_bucket = end_bucket - start_bucket;



  fmt::println(fg(fmt::color::yellow), "=== Results ===");
  bool bucketed_is_faster = duration_bucket.count() < duration_std.count();
  auto bucket_color = bucketed_is_faster ? fg(fmt::color::green) : fg(fmt::color::red);
  double total_time_bucketed = duration_build.count() + duration_bucket.count();
  bool total_bucketed_is_faster = total_time_bucketed < duration_std.count();
  auto total_bucket_color = total_bucketed_is_faster ? fg(fmt::color::green) : fg(fmt::color::red);
  
  fmt::println(fg(fmt::color::cyan), "Total Standard Binary Search:");
  fmt::println(fg(fmt::color::green), "  Total Time:    {:.3f} ms", duration_std.count());
  fmt::println(fg(fmt::color::green), "  Per Query:     {:.6f} ms", duration_std.count() / NUM_QUERIES);

  fmt::println(fg(fmt::color::cyan), "Bucketed Binary Search:");
  fmt::println(bucket_color, "  Total Time:   {:.3f} ms", duration_bucket.count());
  fmt::println(bucket_color, "  Per Query:    {:.6f} ms", duration_bucket.count() / NUM_QUERIES);

  fmt::println(fg(fmt::color::cyan), "Total Bucketed Binary Search (Build + Queries):");
  fmt::println(total_bucket_color, "  Total Time:    {:.3f} ms", total_time_bucketed);
  fmt::println(total_bucket_color, "  Per Query:     {:.6f} ms", total_time_bucketed / NUM_QUERIES);
  
  double build_cost = duration_build.count();
  double std_per_q = duration_std.count() / NUM_QUERIES;
  double bucket_per_q = duration_bucket.count() / NUM_QUERIES;
  double gain_per_q = std_per_q - bucket_per_q;
  double break_even_queries = (gain_per_q > 0) ? (build_cost / gain_per_q) : 0;

  fmt::println(fg(fmt::color::magenta), "\nMin Queries for Profit: {:.0f} queries", break_even_queries);
  fmt::println(fg(fmt::color::magenta), "Min Querries/Data Size: {:.2f}%", (break_even_queries / dataset.data.size()) * 100);
  
  if (bucketed_is_faster) {
    double speedup = duration_std.count() / duration_bucket.count();
    fmt::println(fg(fmt::color::green) | fmt::emphasis::bold, "\n>>> Bucketed Binary Search is FASTER by {:.2f}x! <<<", speedup);
  } else {
    double slowdown = duration_bucket.count() / duration_std.count();
    fmt::println(fg(fmt::color::red) | fmt::emphasis::bold, "\n>>> Bucketed Binary Search is SLOWER by {:.2f}x. <<<", slowdown);
  }; 

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
  else {
    fmt::println(fg(fmt::color::green), "SUCCESS: Checksums match perfectly!");
    fmt::println(fg(fmt::color::green), "Standard Checksum: {}, Bucketed Checksum: {}", checksum_std, checksum_bucket);
  };


  
  dataset.print_params();
  dataset.analyze_index_stats();



  return 0;
};

