#pragma once

#include <map>

using namespace std;

template<class Table>
class RestaurantProcess {
 public:
  RestaurantProcess(double alpha = 0);

  map<Table, int> Get() const;

  void Update(const Table& table, int value);

  double GetLogProbability(const Table& table, double log_p0) const;

  double GetLogProbability(const Table& table, int delta_numerator,
                           int delta_denominator, double log_p0) const;

  bool operator==(const RestaurantProcess<Table>& other) const;

 private:
  double alpha, log_alpha;
  int total_count;
  map<Table, int> table_counts;
};

#include "restaurant_process_inl.h"
