#ifndef _VITERBI_REORDERER_H_
#define _VITERBI_REORDERER_H_

#include "reorderer.h"

using namespace std;

class ViterbiReorderer: public Reorderer {
 public:
  ViterbiReorderer(
      const AlignedTree& tree,
      const Grammar& grammar,
      shared_ptr<RuleStatsReporter> reporter);

 private:
  void Combine(double& cache_prob, double match_prob);

  shared_ptr<pair<Rule, double>> SelectRule(
      const vector<pair<Rule, double>>& candidates);
};

#endif
