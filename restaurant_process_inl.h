#include <cmath>

#include "log_add.h"

template<class Table>
RestaurantProcess<Table>::RestaurantProcess(double alpha) :
    alpha(alpha), log_alpha(log(alpha)), total_count(0) {}

template<class Table>
map<Table, int> RestaurantProcess<Table>::Get() const {
  return table_counts;
}

template<class Table>
void RestaurantProcess<Table>::Update(const Table& table, int value) {
  if (value != 0) {
    auto it = table_counts.find(table);
    if (it == table_counts.end()) {
      table_counts[table] = value;
    } else {
      it->second += value;
      if (it->second == 0) {
        table_counts.erase(it);
      }
    }
    total_count += value;
  }
}

template<class Table>
double RestaurantProcess<Table>::GetLogProbability(
    const Table& table, double log_p0) const {
  auto it = table_counts.find(table);
  int counts = it != table_counts.end() ? it->second : 0;
  double log_numerator = Log<double>::add(log(counts), log_alpha + log_p0);
  double log_denominator = log(total_count + alpha);
  return log_numerator - log_denominator;
}

template<class Table>
double RestaurantProcess<Table>::GetLogProbability(
    const Table& table, int delta_numerator,
    int delta_denominator, double log_p0) const {
  auto it = table_counts.find(table);
  int counts = it != table_counts.end() ? it->second : 0;
  double log_numerator = Log<double>::add(
      log(counts + delta_numerator), log_alpha + log_p0);
  double log_denominator = log(total_count + delta_denominator + alpha);
  return log_numerator - log_denominator;
}

template<class Table>
int RestaurantProcess<Table>::Count(const Table& table) const {
  auto it = table_counts.find(table);
  return it != table_counts.end() ? it->second : 0;
}

template<class Table>
int RestaurantProcess<Table>::GetTotal() const {
  return total_count;
}

template<class Table>
bool RestaurantProcess<Table>::operator==(
    const RestaurantProcess<Table>& other) const {
  return table_counts == other.table_counts &&
         total_count == other.total_count;
}
