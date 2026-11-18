// Search Algorithm 
// Copyright 2026 Daynlight
// Licensed under the GNU General.
// See LICENSE file for details.



#pragma once
#include "i_benchmark_struct.h"



class BenchmarkBSearch : public IBenchmarkStruct<double> {
private:
  std::vector<double> data;
  std::chrono::duration<double, std::milli> prepare_time;
  std::chrono::duration<double, std::milli> search_time;
  long long checksum = 0;



private:
  long long find_nearest(std::vector<double>& data, double target) const {  
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



public:
  BenchmarkBSearch() = default;
  ~BenchmarkBSearch() = default;



public:
  void setData(std::vector<double> data) {
    this->data = std::move(data);
  };



  void freeData() {
    data.clear();
    checksum = 0;
  };



  void prepare() {
    fmt::println(fg(fmt::color::pink), "\nSorting Data");
    auto start_build = std::chrono::high_resolution_clock::now();
    std::sort(data.begin(), data.end());
    auto end_build = std::chrono::high_resolution_clock::now();
    prepare_time = end_build - start_build;
  };



  void search(std::vector<double> test) {
    auto start_std = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < test.size(); ++i) checksum += find_nearest(data, test[i]);
    auto end_std = std::chrono::high_resolution_clock::now();
    search_time = end_std - start_std;
  };



public:
  std::string getName() const {
    return "Binary Search";
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

  };
};

