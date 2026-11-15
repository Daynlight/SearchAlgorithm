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

#include "bucket_search.h"



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

