#include "viterbi_reorderer.h"

ViterbiReorderer::ViterbiReorderer(
    const AlignedTree& tree,
    const Grammar& grammar,
    shared_ptr<RuleStatsReporter> reporter) :
    Reorderer(tree, grammar, reporter) {}

void ViterbiReorderer::Combine(double& cache_prob, double match_prob) {
  cache_prob = max(cache_prob, match_prob);
}

shared_ptr<pair<Rule, double>> ViterbiReorderer::SelectRule(
    const vector<pair<Rule, double>>& candidates) {
  if (candidates.size() == 0) {
    return nullptr;
  }

  pair<Rule, double> max_rule = candidates.front();
  for (auto rule: candidates) {
    if (rule.second > max_rule.second) {
      max_rule = rule;
    }
  }
  return make_shared<pair<Rule, double>>(max_rule);
}
