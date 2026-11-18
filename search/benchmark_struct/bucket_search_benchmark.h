// Search Algorithm 
// Copyright 2026 Daynlight
// Licensed under the GNU General.
// See LICENSE file for details.



#pragma once
#include "i_benchmark_struct.h"
#include "../bucket/bucket_search.h"



class BenchmarkBucketedSearch : public IBenchmarkStruct<double> {
private:
  SortedDataset<double> dataset;
  std::chrono::duration<double, std::milli> prepare_time;
  std::chrono::duration<double, std::milli> search_time;
  long long checksum = 0;



public:
  BenchmarkBucketedSearch() = default;
  ~BenchmarkBucketedSearch() = default;



public:
  void setData(std::vector<double> data) {
    dataset.data = std::move(data);
  };



  void freeData() {
    dataset.clear();
    checksum = 0;
  };



  void prepare() {
    auto start_build = std::chrono::high_resolution_clock::now();
    dataset.build_index();
    auto end_build = std::chrono::high_resolution_clock::now();
    prepare_time = end_build - start_build;
  };



  void search(std::vector<double> test) {
    auto start_std = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < test.size(); ++i) checksum += dataset.find_nearest_bucketed(test[i]);
    auto end_std = std::chrono::high_resolution_clock::now();
    search_time = end_std - start_std;
  };



public:
  std::string getName() const {
    return "Bucketed Search";
  };
  


  double getPrepareTime() const {
    return prepare_time.count();
  };



  double getSearchTime() const {
    return search_time.count();
  };



  long long getChecksum() const {
    return checksum;
  };



  void printInfo() const {
    dataset.print_params();
    dataset.analyze_index_stats();
    dataset.graph_bucket_clasificator_terminal();
  };
};

