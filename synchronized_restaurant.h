#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#include <boost/thread/shared_mutex.hpp>

#include "restaurant.h"
#include "util.h"

using namespace std;

typedef Restaurant<Rule> RuleCounts;

class SynchronizedRuleCounts {
 public:
  SynchronizedRuleCounts(double alpha);

  void AddNonterminal(int nonterminal);

  void Increment(const Rule& rule);

  void Decrement(const Rule& rule);

  double GetLogProbability(const Rule& rule, double p0);

  double GetLogProbability(
      const Rule& rule, int same_rules, int same_tags, double p0);

  vector<int> GetNonterminals();

 private:
  unordered_map<int, RuleCounts> counts;
  unordered_map<int, shared_ptr<boost::shared_mutex>> mutexes;
  mutex general_mutex;

  double alpha;
};
