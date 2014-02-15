#include "multi_sample_reorderer.h"

#include "grammar.h"

#include <iostream>

MultiSampleReorderer::MultiSampleReorderer(
    const AlignedTree& tree,
    const Grammar& grammar,
    shared_ptr<RuleStatsReporter> reporter,
    RandomGenerator& generator,
    unsigned int num_iterations,
    unsigned int max_candidates) :
    reorderer(tree, grammar, reporter, generator),
    num_iterations(num_iterations),
    max_candidates(max_candidates) {}

Distribution MultiSampleReorderer::GetDistribution() {
  map<String, int> reordering_counts;
  for (unsigned int i = 0; i < num_iterations; ++i) {
    ++reordering_counts[reorderer.ConstructReordering()];
  }

  set<pair<int, String>> candidates;
  for (const auto& reordering: reordering_counts) {
    candidates.insert(make_pair(reordering.second, reordering.first));
  }

  while (candidates.size() > max_candidates) {
    candidates.erase(candidates.begin());
  }

  double total_counts = 0;
  for (const auto& reordering: candidates) {
    total_counts += reordering.first;
  }

  Distribution result;
  for (const auto& reordering: candidates) {
    double prob = reordering.first / total_counts;
    result.push_back(make_pair(reordering.second, prob));
  }

  return result;
}
