// Search Algorithm 
// Copyright 2026 Daynlight
// Licensed under the GNU General.
// See LICENSE file for details.



#pragma once
#include <string>
#include <vector>



class IBenchmarkEnv {
public:
  virtual std::string getName() = 0;
public:
  virtual std::vector<double> prepare_data() = 0;
  virtual std::vector<double> prepare_testing_data() = 0;
};

