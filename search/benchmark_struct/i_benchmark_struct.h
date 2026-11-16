#pragma once

#include <vector>
#include <string>
#include <utility>



template<typename T>
class IBenchmarkStruct {
public:
  virtual void setData(std::vector<T> data) = 0;
  virtual void freeData() = 0;
  virtual void prepare() = 0;
  virtual void search(std::vector<T> test) = 0;

public:
  virtual std::string getName() const = 0;
  virtual double getPrepareTime() const = 0;
  virtual double getSearchTime() const = 0;
  virtual long long getChecksum() const = 0;
  
public:
  virtual void printInfo() const = 0;
};

