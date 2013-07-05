#include "viterbi_reorderer.h"

ViterbiReorderer::ViterbiReorderer(shared_ptr<Grammar> grammar) :
    Reorderer(grammar) {}

void ViterbiReorderer::Combine(double& cache_prob, double match_prob) {
  cache_prob = max(cache_prob, match_prob);
}

shared_ptr<Rule> ViterbiReorderer::SelectRule(
    const vector<pair<Rule, double>>& candidates) {
  double max_prob = -numeric_limits<double>::infinity();
  shared_ptr<Rule> max_rule;
  for (auto rule: candidates) {
    if (rule.second > max_prob) {
      max_prob = rule.second;
      max_rule = make_shared<Rule>(rule.first);
    }
  }
  return max_rule;
}
