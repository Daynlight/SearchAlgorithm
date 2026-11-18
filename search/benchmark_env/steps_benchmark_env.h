// Search Algorithm 
// Copyright 2026 Daynlight
// Licensed under the GNU General.
// See LICENSE file for details.



#pragma once
#include <random>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <map>
#include <chrono>
#include <string>
#include <cmath>
#include <memory>

#include <fmt/base.h>
#include <fmt/color.h>
#include <fmt/ranges.h>
#include <indicators/progress_bar.hpp>

#include "i_benchmark_env.h"



class StepsBenchmarkEnv : public IBenchmarkEnv {
public:
  StepsBenchmarkEnv() = default;
  ~StepsBenchmarkEnv() = default;



public:
  std::string getName() {
    return "Step Distribution";
  };



public:
  std::vector<double> prepare_data(){
    std::string filename = DATA_FOLDER + std::string("steps.data.bin");
    if(!std::filesystem::exists(DATA_FOLDER)) std::filesystem::create_directory(DATA_FOLDER);
      
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
      
      std::random_device dev;
      std::mt19937 rng(dev());
      std::uniform_real_distribution<double> dist_dense(DATA_DENSE_MIN_VAL, DATA_DENSE_MAX_VAL);
      
      fmt::println(fg(fmt::color::pink), "\nGenerating Skewed Data with Outliers");
      for(long long i = 0; i < DATA_SIZE; i++) {
        if      (i < DATA_SIZE * 0.25) data.emplace_back(dist_dense(rng));
        else if (i < DATA_SIZE * 0.50) data.emplace_back(dist_dense(rng) + 100);
        else if (i < DATA_SIZE * 0.75) data.emplace_back(dist_dense(rng) + 200);
        else data.emplace_back(dist_dense(rng) + 300);
        if (i % 10000000 == 0) bar.set_progress(i);
      };
      fmt::print("\n");
  
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
  
    fmt::println(fg(fmt::color::pink), "Generating Queries for Offset Case");
    std::uniform_real_distribution<double> dist_dense(DATA_DENSE_MIN_VAL, DATA_DENSE_MAX_VAL);
    std::bernoulli_distribution choice(0.99); 
  
    fmt::println(fg(fmt::color::pink), "\nGenerating Skewed Data with Outliers");
    for(long long i = 0; i < NUM_QUERIES; i++) {
      if      (i < NUM_QUERIES * 0.25) test_targets.emplace_back(dist_dense(rng));
      else if (i < NUM_QUERIES * 0.50) test_targets.emplace_back(dist_dense(rng) + 100);
      else if (i < NUM_QUERIES * 0.75) test_targets.emplace_back(dist_dense(rng) + 200);
      else                             test_targets.emplace_back(dist_dense(rng) + 300);
    };
  
    return test_targets;
  };
};

