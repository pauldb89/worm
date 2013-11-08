#ifndef _MULTI_SAMPLE_REORDERER_H_
#define _MULTI_SAMPLE_REORDERER_H_

#include "reorderer_base.h"
#include "single_sample_reorderer.h"

class Grammar;

class MultiSampleReorderer: public ReordererBase {
 public:
  MultiSampleReorderer(shared_ptr<Grammar> grammar,
                       RandomGenerator& generator,
                       unsigned int num_iterations);

  String Reorder(const AlignedTree& tree);

 private:
  SingleSampleReorderer reorderer;
  unsigned int num_iterations;
};

#endif
