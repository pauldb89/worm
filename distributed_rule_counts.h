#pragma once

#include <memory>
#include <unordered_map>

#include "restaurant_process.h"
#include "util.h"

using namespace std;

typedef unordered_map<int, RestaurantProcess<Rule>> RuleCounts;

class DistributedRuleCounts {
 public:
  DistributedRuleCounts(int max_threads, double alpha);

  void AddNonterminal(int nonterminal);

  void Increment(const Rule& rule);

  void Decrement(const Rule& rule);

  double GetLogProbability(const Rule& rule, double p0);

  double GetLogProbability(
      const Rule& rule, int same_rules, int same_tags, double p0);

  vector<int> GetNonterminals();

  int Count(const Rule& rule) const;

  int Count(int nonterminal) const;

  void Synchronize();

 private:
  void AddNonterminal(vector<RuleCounts>& rule_counts, int nonterminal);

  void UpdateCounts(RuleCounts& total_counts,
                    const RuleCounts& rule_counts,
                    int factor);

  void UpdateCounts(RuleCounts& total_counts,
                    const vector<RuleCounts>& rule_counts,
                    int factor);

  vector<RuleCounts> rule_counts;
  RuleCounts snapshot;

  double alpha;
};
