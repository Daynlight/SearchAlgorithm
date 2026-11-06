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
#define NUM_QUERIES 50000000
#define BUCKET_SIZE 10000000



struct BucketIndex {
  double min_val{0};
  double max_val{0};
  double bucket_width{1.0};
  double inv_bucket_width{1.0};
  std::vector<std::pair<long long, long long>> buckets; 
};



struct SortedDataset {
  std::vector<double> data;
  BucketIndex bucket_index;

  void build_index(long long num_buckets = 10000) {
    if (data.empty()) return;
    bucket_index.min_val = data.front();
    bucket_index.max_val = data.back();
    bucket_index.bucket_width = (bucket_index.max_val - bucket_index.min_val) / num_buckets;
    if (bucket_index.bucket_width <= 0) bucket_index.bucket_width = 1.0;
    
    bucket_index.inv_bucket_width = 1.0 / bucket_index.bucket_width;
    bucket_index.buckets.assign(num_buckets, {-1, -1});

    for (long long i = 0; i < static_cast<long long>(data.size()); ++i) {
      long long b_id = static_cast<long long>((data[i] - bucket_index.min_val) * bucket_index.inv_bucket_width);
      if (b_id >= num_buckets) b_id = num_buckets - 1;
      if (b_id < 0) b_id = 0;

      if (bucket_index.buckets[b_id].first == -1) bucket_index.buckets[b_id] = {i, i};
      else bucket_index.buckets[b_id].second = i;
    };

    long long prev_start = 0;
    long long prev_end = 0;
    for (long long i = 0; i < num_buckets; ++i) {
      if (bucket_index.buckets[i].first != -1) {
        prev_start = bucket_index.buckets[i].first;
        prev_end = bucket_index.buckets[i].second;
        break;
      };
    };

    for (long long i = 0; i < num_buckets; ++i) {
      if (bucket_index.buckets[i].first == -1) bucket_index.buckets[i] = {prev_start, prev_end};
      else {
        prev_start = bucket_index.buckets[i].first;
        prev_end = bucket_index.buckets[i].second;
      };
    };
  };

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

  long long find_nearest_bucketed(double target) const {
    if (data.empty()) return -1;

    long long left = 0;
    long long right = static_cast<long long>(data.size()) - 1;

    if (target <= data[left]) return left;
    if (target >= data[right]) return right;
 
    long long b_id = static_cast<long long>((target - bucket_index.min_val) * bucket_index.inv_bucket_width);
    b_id = std::clamp(b_id, 0LL, static_cast<long long>(bucket_index.buckets.size()) - 1);
    
    auto [best_start, best_end] = bucket_index.buckets[b_id];

    left = best_start;
    right = best_end;

    while (left <= right) {
      long long mid = left + (right - left) / 2;
      if (data[mid] == target) return mid;
      else if (data[mid] < target) left = mid + 1;
      else right = mid - 1;
    };

    if (right < best_start) return best_start;
    if (left > best_end) return best_end;

    if (std::abs(data[left] - target) < std::abs(data[right] - target)) return left;
    return right;
  };
};



int main(){
  fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "========================");
  fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "=== Seach Algorithms ===");
  fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "========================");
  


  fmt::println(fg(fmt::color::yellow), "=== Prepare Data");
  const std::string filename = "data.bin";
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
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<double> dist(DATA_MIN_VAL, DATA_MAX_VAL);
    
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
    for(long long i = 0; i < DATA_SIZE; i++) {
      data.emplace_back(dist(rng));
      
      if (i % 10000 == 0) bar.set_progress(i);
    };
    fmt::print("\n");

    fmt::println(fg(fmt::color::pink), "\nSorting Data");
    std::sort(data.begin(), data.end());

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
    std::uniform_real_distribution<double> dist(DATA_MIN_VAL, DATA_MAX_VAL);
    for(int i = 0; i < NUM_QUERIES; ++i) test_targets.push_back(dist(rng));
  }

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

  if (checksum_std == -999999) fmt::print("");
  if (checksum_bucket == -999999) fmt::print("");
  
  return 0;
};

