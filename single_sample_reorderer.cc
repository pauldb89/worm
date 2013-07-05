#include "single_sample_reorderer.h"

#include "log_add.h"

SingleSampleReorderer::SingleSampleReorderer(
    shared_ptr<Grammar> grammar, RandomGenerator& generator) :
    Reorderer(grammar),
    generator(generator),
    uniform_distribution(0, 1) {}

void SingleSampleReorderer::Combine(double& cache_prob, double match_prob) {
  cache_prob += match_prob;
}

shared_ptr<Rule> SingleSampleReorderer::SelectRule(
    const vector<pair<Rule, double>>& candidates) {
  double total_prob = Log<double>::zero();
  for (auto rule: candidates) {
    total_prob = Log<double>::add(total_prob, rule.second);
  }

  double r = log(uniform_distribution(generator)) + total_prob;
  for (auto rule: candidates) {
    if (rule.second >= r) {
      return make_shared<Rule>(rule.first);
    }
    r = Log<double>::subtract(r, rule.second);
  }

  return nullptr;
}
