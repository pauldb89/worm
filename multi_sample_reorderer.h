#ifndef _MULTI_SAMPLE_REORDERER_H_
#define _MULTI_SAMPLE_REORDERER_H_

#include "reorderer_base.h"
#include "single_sample_reorderer.h"

class Grammar;

class MultiSampleReorderer {
 public:
  MultiSampleReorderer(
      const AlignedTree& tree,
      const Grammar& grammar,
      shared_ptr<RuleStatsReporter> reporter,
      RandomGenerator& generator,
      unsigned int num_iterations,
      unsigned int max_candidates);

  Distribution GetDistribution();

 private:
  AlignedTree tree;
  SingleSampleReorderer reorderer;
  unsigned int num_iterations, max_candidates;
};

#endif
