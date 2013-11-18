#pragma once

#include <map>

#include "util.h"

using namespace std;

class RuleStatsReporter {
 public:
  void UpdateRuleStats(const pair<Rule, double>& rule);

  void DisplayRuleStats(ostream& stream, Dictionary& dictionary);

 private:
  // Checks if the STSG rule actually implies any kind of reordering.
  bool IsReorderingRule(const Rule& rule);

  map<Rule, int> rule_counts;
  map<Rule, double> rule_probs;
};
