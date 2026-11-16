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



class LinearBenchmarkEnv : public IBenchmarkEnv {
public:
  LinearBenchmarkEnv() = default;
  ~LinearBenchmarkEnv() = default;



public:
  std::string getName() {
    return "Linear Distribution";
  };



public:
  std::vector<double> prepare_data(){
    std::string filename = DATA_FOLDER + std::string("data.bin");
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
      std::uniform_real_distribution<double> dist(DATA_MIN_VAL, DATA_MAX_VAL);
      
      for(long long i = 0; i < DATA_SIZE; i++) {
        data.emplace_back(dist(rng)); 
        if (i % 10000 == 0) bar.set_progress(i);
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

    fmt::println(fg(fmt::color::pink), "Generating Queries for Standard Case");
    std::uniform_real_distribution<double> dist(DATA_MIN_VAL, DATA_MAX_VAL);
    for(int i = 0; i < NUM_QUERIES; ++i) test_targets.push_back(dist(rng));
  
    return test_targets;
  };
};

