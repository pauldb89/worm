#ifndef _SINGLE_SAMPLE_REORDERER_H_
#define _SINGLE_SAMPLE_REORDERER_H_

#include <random>

#include "reorderer.h"

using namespace std;

typedef mt19937 RandomGenerator;

class SingleSampleReorderer: public Reorderer {
 public:
  SingleSampleReorderer(shared_ptr<Grammar> grammar,
                        RandomGenerator& generator);

 private:
  void Combine(double& cache_prob, double match_prob);

  shared_ptr<Rule> SelectRule(const vector<pair<Rule, double>>& candidates);

  RandomGenerator& generator;
  uniform_real_distribution<double> uniform_distribution;
};

#endif
