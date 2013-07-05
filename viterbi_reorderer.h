#ifndef _VITERBI_REORDERER_H_
#define _VITERBI_REORDERER_H_

#include "reorderer.h"

using namespace std;

class ViterbiReorderer: public Reorderer {
 public:
  ViterbiReorderer(shared_ptr<Grammar> grammar);

 private:
  void Combine(double& cache_prob, double match_prob);

  shared_ptr<Rule> SelectRule(const vector<pair<Rule, double>>& candidates);
};

#endif
