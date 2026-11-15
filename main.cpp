#include <random>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <map>
#include <chrono>
#include <string>
#include <cmath>

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
#define BUCKET_SIZE 100000000 



struct BucketIndex {
  double min_val{0};
  double max_val{0};
  double range_span{1.0};

  std::vector<double> slopes;
  std::vector<double> intercepts;

  std::vector<long long> bucket_start;
  std::vector<long long> counts;
};



struct SortedDataset {
  std::vector<double> data;
  BucketIndex bucket_index;

  // 2 op, O(1)
  inline double fast_abs(double x) const {
    union {
      double d;
      uint64_t i;
    } u;
    u.d = x;                                                                    // 1
    u.i &= 0x7FFFFFFFFFFFFFFFULL;                                               // 1
    return u.d;                                                                 // 0
  }



  // O(1)
  inline long long find_segment(double val) const {
    const long long num_segs = static_cast<long long>(bucket_index.slopes.size());
    if (num_segs <= 1) return 0;
    if (val <= bucket_index.min_val) return 0;
    if (val >= bucket_index.max_val) return num_segs - 1;

    double normalized = (val - bucket_index.min_val) / bucket_index.range_span;
    long long seg = static_cast<long long>(normalized * num_segs);
    
    if (seg < 0) return 0;
    if (seg >= num_segs) return num_segs - 1;
    return seg;
  }



  // O(1)
  long long bucket_clasificator(double val) const {
    long long seg = find_segment(val);
    double estimated = bucket_index.slopes[seg] * val + bucket_index.intercepts[seg];
    long long b_id = static_cast<long long>(estimated);
    long long max_b_idx = static_cast<long long>(bucket_index.bucket_start.size()) - 1;
    return (b_id < 0) ? 0 : (b_id > max_b_idx ? max_b_idx : b_id);
  }



#define CLASSIFY_FAST(val, min_val, range_span, num_segs, slopes_ptr, intercepts_ptr, max_b_idx, b_out) \
  do { \
    double v_ = (val); \
    long long seg_ = 0; \
    if (v_ <= (min_val)) { \
      seg_ = 0; \
    } else if (v_ >= (min_val) + (range_span)) { \
      seg_ = (num_segs) - 1; \
    } else { \
      double norm_ = (v_ - (min_val)) / (range_span); \
      seg_ = static_cast<long long>(norm_ * (num_segs)); \
      if (seg_ < 0) seg_ = 0; \
      if (seg_ >= (num_segs)) seg_ = (num_segs) - 1; \
    } \
    double est_ = (slopes_ptr)[seg_] * v_ + (intercepts_ptr)[seg_]; \
    long long b_macro_ = static_cast<long long>(est_); \
    (b_out) = (b_macro_ < 0) ? 0 : (b_macro_ > (max_b_idx) ? (max_b_idx) : b_macro_); \
  } while(0)



  // O(n)
  void build_index(long long num_buckets = 0, long long target_segment_size = 64) {      
    if (data.empty()) return;
    long long n = data.size();
    if (num_buckets == 0) num_buckets = n;

    bucket_index.min_val = data.front();
    bucket_index.max_val = data.back();
    bucket_index.range_span = bucket_index.max_val - bucket_index.min_val;
    if (bucket_index.range_span <= 0) bucket_index.range_span = 1.0;

    long long num_segments = n / target_segment_size;
    if (num_segments < 1000) num_segments = 1000;
    if (num_segments > 65536) num_segments = 65536; 

    bucket_index.slopes.resize(num_segments);
    bucket_index.intercepts.resize(num_segments);

    double seg_width = bucket_index.range_span / num_segments;
    double prev_y = 0.0;

    long long data_cursor = 0;
    for (long long s = 0; s < num_segments; ++s) {
      double x_first = bucket_index.min_val + s * seg_width;
      double x_last = (s == num_segments - 1) ? bucket_index.max_val : (bucket_index.min_val + (s + 1) * seg_width);

      auto it = std::lower_bound(data.begin() + data_cursor, data.end(), x_last);
      long long end_idx = std::distance(data.begin(), it);
      if (end_idx >= n) end_idx = n - 1;
      data_cursor = end_idx; 

      double dx = x_last - x_first;
      double y_start = prev_y;
      double y_end = static_cast<double>(end_idx);

      double slope, intersect;
      if (dx > 1e-9) {
        slope = (y_end - y_start) / dx;
        intersect = y_start - slope * x_first;
      } else {
        slope = 0.0;
        intersect = y_start;
      };

      bucket_index.slopes[s] = slope;
      bucket_index.intercepts[s] = intersect;
      prev_y = slope * x_last + intersect;
    };

    bucket_index.bucket_start.resize(num_buckets);
    bucket_index.counts.assign(num_buckets, 0);

    const double* d_ptr = data.data();
    long long* start_ptr = bucket_index.bucket_start.data();
    long long* count_ptr = bucket_index.counts.data();
    
    const double min_v = bucket_index.min_val;
    const double span_v = bucket_index.range_span;
    const double* slopes_ptr = bucket_index.slopes.data();
    const double* intercepts_ptr = bucket_index.intercepts.data();
    const long long max_b_idx = num_buckets - 1;

    long long i = 0;
    for (; i <= n - 8; i += 8) {
      long long b0, b1, b2, b3, b4, b5, b6, b7;

      CLASSIFY_FAST(d_ptr[i],   min_v, span_v, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b0);
      CLASSIFY_FAST(d_ptr[i+1], min_v, span_v, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b1);
      CLASSIFY_FAST(d_ptr[i+2], min_v, span_v, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b2);
      CLASSIFY_FAST(d_ptr[i+3], min_v, span_v, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b3);
      CLASSIFY_FAST(d_ptr[i+4], min_v, span_v, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b4);
      CLASSIFY_FAST(d_ptr[i+5], min_v, span_v, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b5);
      CLASSIFY_FAST(d_ptr[i+6], min_v, span_v, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b6);
      CLASSIFY_FAST(d_ptr[i+7], min_v, span_v, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b7);

      count_ptr[b0]++; count_ptr[b1]++; count_ptr[b2]++; count_ptr[b3]++;
      count_ptr[b4]++; count_ptr[b5]++; count_ptr[b6]++; count_ptr[b7]++;
    }

    for (; i < n; ++i) {
      long long b_id;
      CLASSIFY_FAST(d_ptr[i], min_v, span_v, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b_id);
      count_ptr[b_id]++;
    }

    long long current_pos = 0;
    for (long long b = 0; b < num_buckets; ++b) {
      long long cnt = count_ptr[b];
      if (cnt > 0) {
        start_ptr[b] = current_pos;
        current_pos += cnt;
      } 
      else start_ptr[b] = -1;
    }
  }



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

    long long b_id = bucket_clasificator(target);

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
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold, "\n=== Learned Index Parameters (O(1) Piecewise Linear) ===\n");
    fmt::print(fmt::fg(fmt::color::green),  "  Dataset Size:        {}\n", data.size());
    fmt::print(fmt::fg(fmt::color::yellow), "  Min / Max Value:     {:.4f} / {:.4f}\n", bucket_index.min_val, bucket_index.max_val);
    fmt::print(fmt::fg(fmt::color::yellow), "  Range Span:          {:.4f}\n", bucket_index.range_span);
    fmt::print(fmt::fg(fmt::color::yellow), "  Segments Count:      {}\n", bucket_index.slopes.size());
    fmt::print(fmt::fg(fmt::color::magenta), "  Total Buckets:       {}\n", bucket_index.bucket_start.size());
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



  void graph_bucket_clasificator_terminal(int width = 80, int height = 24) {
    const auto& index = bucket_index;
    if (index.slopes.empty() || index.intercepts.empty() || index.bucket_start.empty()) {
      fmt::print(fmt::fg(fmt::color::red), "Index is empty, cannot draw graph.\n");
      return;
    }
  
    const double min_val = index.min_val;
    const double max_val = index.max_val;
    const double span = index.range_span;
    const long long max_b_idx = static_cast<long long>(index.bucket_start.size()) - 1;
  
    if (max_b_idx <= 0 || span <= 0.0) {
      fmt::print(fmt::fg(fmt::color::red), "Invalid index range for plotting.\n");
      return;
    }
  
    std::vector<std::string> canvas(height, std::string(width, ' '));
    std::vector<long long> y_samples(width, 0);
  
    for (int col = 0; col < width; ++col) {
      double ratio = (width > 1) ? static_cast<double>(col) / (width - 1) : 0.0;
      double x = min_val + ratio * span;
      long long b_id = bucket_clasificator(x);
      y_samples[col] = b_id;
    }
  
    for (int col = 0; col < width; ++col) {
      int row = static_cast<int>(std::round((static_cast<double>(y_samples[col]) / max_b_idx) * (height - 1)));
      row = std::clamp(row, 0, height - 1);
      int screen_y = (height - 1) - row; 
  
      canvas[screen_y][col] = '*';
  
      if (col > 0) {
        int prev_row = static_cast<int>(std::round((static_cast<double>(y_samples[col - 1]) / max_b_idx) * (height - 1)));
        prev_row = std::clamp(prev_row, 0, height - 1);
        int prev_screen_y = (height - 1) - prev_row;
  
        int y_start = std::min(screen_y, prev_screen_y);
        int y_end = std::max(screen_y, prev_screen_y);
        for (int y = y_start + 1; y < y_end; ++y) {
          if (canvas[y][col] == ' ') {
            canvas[y][col] = '|';
          }
        }
      }
    }
  
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "\n=== Classifier Graph: bucket_clasificator(x) [Segments: {}] ===\n",
               index.slopes.size());
  
    for (int r = 0; r < height; ++r) {
      double y_label_val = static_cast<double>(max_b_idx) * (height - 1 - r) / (height - 1);
      fmt::print(fmt::fg(fmt::color::yellow), "{:>10.0f} |", y_label_val);
      fmt::print(fmt::fg(fmt::color::green), "{}\n", canvas[r]);
    }
  
    fmt::print(fmt::fg(fmt::color::yellow), "{:>10} +{}\n", "", std::string(width, '-'));
    
    double mid_val = min_val + span * 0.5;
    std::string x_labels = fmt::format("{:<16.2f}{:^48.2f}{:>16.2f}", min_val, mid_val, max_val);
    fmt::print(fmt::fg(fmt::color::cyan), "{:>12}{}\n\n", "", x_labels);
  };
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
  dataset.graph_bucket_clasificator_terminal(50, 50);



  return 0;
};

