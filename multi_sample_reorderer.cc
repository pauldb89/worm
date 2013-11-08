#include "multi_sample_reorderer.h"

#include "grammar.h"

MultiSampleReorderer::MultiSampleReorderer(
    shared_ptr<Grammar> grammar,
    RandomGenerator& generator,
    unsigned int num_iterations) :
    reorderer(grammar, generator),
    num_iterations(num_iterations) {}

String MultiSampleReorderer::Reorder(const AlignedTree& tree) {
  map<String, int> reordering_counts;
  for (unsigned int i = 0; i < num_iterations; ++i) {
    ++reordering_counts[reorderer.Reorder(tree)];
  }

  String result;
  int max_counts = 0;
  for (auto reordering: reordering_counts) {
    if (reordering.second > max_counts) {
      max_counts = reordering.second;
      result = reordering.first;
    }
  }

  return result;
}
