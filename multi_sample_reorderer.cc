#include "multi_sample_reorderer.h"

#include "grammar.h"

MultiSampleReorderer::MultiSampleReorderer(
    const AlignedTree& tree,
    const Grammar& grammar,
    shared_ptr<RuleStatsReporter> reporter,
    RandomGenerator& generator,
    unsigned int num_iterations) :
    reorderer(tree, grammar, reporter, generator),
    num_iterations(num_iterations) {}

String MultiSampleReorderer::ConstructReordering() {
  map<String, int> reordering_counts;
  for (unsigned int i = 0; i < num_iterations; ++i) {
    ++reordering_counts[reorderer.ConstructReordering()];
  }

  String result;
  int max_counts = 0;
  for (const auto& reordering: reordering_counts) {
    if (reordering.second > max_counts) {
      max_counts = reordering.second;
      result = reordering.first;
    }
  }

  return result;
}
