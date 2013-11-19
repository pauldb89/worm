#include "single_sample_reorderer.h"

#include "log_add.h"

SingleSampleReorderer::SingleSampleReorderer(
    const AlignedTree& tree,
    const Grammar& grammar,
    shared_ptr<RuleStatsReporter> reporter,
    RandomGenerator& generator) :
    Reorderer(tree, grammar, reporter),
    generator(generator),
    uniform_distribution(0, 1) {
  ConstructProbabilityCache();
}

void SingleSampleReorderer::Combine(double& cache_prob, double match_prob) {
  cache_prob = Log<double>::add(cache_prob, match_prob);
}

shared_ptr<pair<Rule, double>> SingleSampleReorderer::SelectRule(
    const vector<pair<Rule, double>>& candidates) {
  if (candidates.size() == 0) {
    return nullptr;
  }

  double total_prob = Log<double>::zero();
  for (auto rule: candidates) {
    total_prob = Log<double>::add(total_prob, rule.second);
  }

  double r = log(uniform_distribution(generator)) + total_prob;
  for (auto rule: candidates) {
    if (rule.second >= r) {
      return make_shared<pair<Rule, double>>(rule);
    }
    r = Log<double>::subtract(r, rule.second);
  }

  assert(false);
  return nullptr;
}
