#include "reorderer.h"

#include "aligned_tree.h"
#include "util.h"

String Reorderer::Reorder(const AlignedTree& tree) {
  Cache cache = ConstructProbabilityCache(tree);
  return ConstructReordering(cache, tree);
}

Cache Reorderer::ConstructProbabilityCache(const AlignedTree& tree) {
  Cache cache;
  return cache;
}

String Reorderer::ConstructReordering(const Cache& cache,
                                      const AlignedTree& tree) {
  String reordering;
  return reordering;
}
