// Search Algorithm 
// Copyright 2026 Daynlight
// Licensed under the GNU General.
// See LICENSE file for details.



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

#define DATA_FOLDER "data/"
#define DATA_SIZE 10000000 
#define DATA_MIN_VAL -20000000
#define DATA_MAX_VAL 20000000

#define DATA_DENSE_MIN_VAL -10
#define DATA_DENSE_MAX_VAL 10

#define NUM_QUERIES 10000000

#include "benchmark_struct/i_benchmark_struct.h"
#include "benchmark_struct/bucket_search_benchmark.h"
#include "benchmark_struct/b_search_benchmark.h"

#include "benchmark_env/i_benchmark_env.h"
#include "benchmark_env/linear_benchmark_env.h"
#include "benchmark_env/steps_benchmark_env.h"
#include "benchmark_env/outliner_benchmark_env.h"



void run(std::vector<std::unique_ptr<IBenchmarkStruct<double>>>& benchmark_structures, std::vector<std::unique_ptr<IBenchmarkEnv>>& benchmark_envs) {
  for(std::unique_ptr<IBenchmarkEnv>& env: benchmark_envs) {
    fmt::println(fg(fmt::color::light_salmon), "=== {} Enviroment ===", env->getName());
    fmt::println(fg(fmt::color::yellow), "=== Prepare Data");
    std::vector<double> data = env->prepare_data();
    fmt::println("");
    fmt::println(fg(fmt::color::pink), "size: {}", data.size());

    fmt::println(fg(fmt::color::yellow), "=== Prepare Testing Data");
    std::vector<double> test_targets = env->prepare_testing_data();
    fmt::println(""); 

    
    for(std::unique_ptr<IBenchmarkStruct<double>>& el : benchmark_structures) {
      fmt::println(fg(fmt::color::yellow), "=== {} Setting Data ===", el->getName());
      el->setData(data);

      fmt::println(fg(fmt::color::yellow), "=== {} Prepare Structure ===", el->getName());
      el->prepare(); 
      fmt::println(fg(fmt::color::green), "{} Prepare Time: {:.3f} ms", el->getName(), el->getPrepareTime());  

      fmt::println(fg(fmt::color::yellow), "=== {} Running Benchmarks ({} queries) ===", el->getName(), test_targets.size());
      el->search(test_targets);
      fmt::println(fg(fmt::color::green), "{} Querry Time: {:.3f} ms", el->getName(), el->getSearchTime());  
    };

    
    fmt::println(fg(fmt::color::yellow), "=== Results ===");
    for(const std::unique_ptr<IBenchmarkStruct<double>>& el : benchmark_structures) {
      double search_time = el->getSearchTime();
      double prepare_time = el->getPrepareTime();
      double total_time = search_time + prepare_time;

      fmt::println(fg(fmt::color::blue), "=== Results for {} ===", el->getName());
      fmt::println(fg(fmt::color::cyan), "Querry {}:", el->getName());
      fmt::println(fg(fmt::color::pink), "  Total Time:   {:.3f} ms", search_time);
      fmt::println(fg(fmt::color::pink), "  Per Query:    {:.6f} ms", search_time / test_targets.size());

      fmt::println(fg(fmt::color::cyan), "Querry + Prepare {}:", el->getName());
      fmt::println(fg(fmt::color::pink), "  Total Time:    {:.3f} ms", total_time);
      fmt::println(fg(fmt::color::pink), "  Per Query:     {:.6f} ms", total_time / test_targets.size());
    };


    
    std::vector<IBenchmarkStruct<double>*> ranked;
    ranked.reserve(benchmark_structures.size());
    for (const auto& el : benchmark_structures) ranked.push_back(el.get());

    std::sort(ranked.begin(), ranked.end(), [](const auto* a, const auto* b) {
      return (a->getSearchTime() + a->getPrepareTime()) < (b->getSearchTime() + b->getPrepareTime());
    });

    fmt::println(fg(fmt::color::cyan) | fmt::emphasis::bold, "\n=== Performance Leaderboard & Gaps ===");
    for (size_t rank = 0; rank < ranked.size(); ++rank) {
      double q_time = ranked[rank]->getSearchTime();
      double prep_time = ranked[rank]->getPrepareTime();
      double tot_time = q_time + prep_time;

      auto color = (rank == 0) ? fg(fmt::color::green) | fmt::emphasis::bold : fg(fmt::color::white);
      fmt::println(color, "#{:<2} | {:<30} | Query Time: {:>10.3f} ms | Total Time: {:>10.3f} ms", rank + 1, ranked[rank]->getName(), q_time, tot_time);

      if (rank + 1 < ranked.size()) {
        auto* faster = ranked[rank];
        auto* slower = ranked[rank + 1];

        double t_fast = faster->getSearchTime();
        double t_slow = slower->getSearchTime();
        
        double diff_ms = t_slow - t_fast;
        double speedup = t_slow / t_fast;

        fmt::println(fg(fmt::color::green), "     └─ vs #{}: FASTER by {:.2f}x (saved {:.3f} ms)", rank + 2, speedup, diff_ms);
        
        double q_per_fast = faster->getSearchTime() / test_targets.size();
        double q_per_slow = slower->getSearchTime() / test_targets.size();
        double prep_diff = std::abs(faster->getPrepareTime() - slower->getPrepareTime());
        double gain_per_q = std::abs(q_per_slow - q_per_fast);

        if (faster->getSearchTime() < slower->getSearchTime() && faster->getPrepareTime() > slower->getPrepareTime()) {
          double break_even = prep_diff / gain_per_q;
          fmt::println(fg(fmt::color::magenta), "     └─ Min Queries for Profit: {:.0f} queries ({:.2f}% of dataset)", break_even, (break_even / data.size()) * 100.0);
        } else {
          fmt::println(fg(fmt::color::magenta), "     └─ Dominates from query #1 (better build time & query time)");
        };
      };
    };
    fmt::println("");



    bool all_checksums_eq = true;
    if(!benchmark_structures.empty()) {
      long long checksum = benchmark_structures[0]->getChecksum();
      for(const std::unique_ptr<IBenchmarkStruct<double>>& el : benchmark_structures) {
        if(checksum != el->getChecksum()) {
          all_checksums_eq = false;
          break;
        };
      };
    };
    
    auto checksum_color = all_checksums_eq ? fg(fmt::color::green) : fg(fmt::color::red);

    if (!all_checksums_eq)  fmt::println(fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: Checksums do not match! Bucketed search returned wrong results!"); 
    else                    fmt::println(fg(fmt::color::green), "SUCCESS: Checksums match perfectly!");
    
    for(const std::unique_ptr<IBenchmarkStruct<double>>& el : benchmark_structures) {
      fmt::println(checksum_color , "{} Checksum: {}", el->getName(), el->getChecksum());
    };


    for(const std::unique_ptr<IBenchmarkStruct<double>>& el : benchmark_structures) {
      fmt::println(fg(fmt::color::yellow), "=== {} Info ===", el->getName());
      el->printInfo();
    };

    for(std::unique_ptr<IBenchmarkStruct<double>>& el : benchmark_structures) el->freeData();
  };
};






int main(){
  fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "========================");
  fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "=== Seach Algorithms ===");
  fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "========================");
      
  
  fmt::println(fg(fmt::color::yellow), "=== Creating Benchmark Envitoments ===");
  std::vector<std::unique_ptr<IBenchmarkEnv>> benchmark_envs = {};
  benchmark_envs.emplace_back(std::make_unique<LinearBenchmarkEnv>());
  benchmark_envs.emplace_back(std::make_unique<StepsBenchmarkEnv>());
  benchmark_envs.emplace_back(std::make_unique<OutlinerBenchmarkEnv>());


  fmt::println(fg(fmt::color::yellow), "=== Creating Benchmark Structure ===");
  std::vector<std::unique_ptr<IBenchmarkStruct<double>>> benchmark_structures = {};
  benchmark_structures.emplace_back(std::make_unique<BenchmarkBSearch>());
  benchmark_structures.emplace_back(std::make_unique<BenchmarkBucketedSearch>());


  run(benchmark_structures, benchmark_envs);


  return 0;
};

