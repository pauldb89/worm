#ifndef _REORDERER_H_
#define _REORDERER_H_

#include <map>

#include "grammar.h"
#include "reorderer_base.h"

using namespace std;

typedef AlignedTree::iterator NodeIter;
typedef map<NodeIter, double> Cache;

class RuleStatsReporter;

class Reorderer : public ReordererBase {
 public:
  Reorderer(
      const AlignedTree& tree,
      const Grammar& grammar,
      shared_ptr<RuleStatsReporter> reporter);

  String ConstructReordering();

 protected:
  void ConstructProbabilityCache();

  virtual void Combine(double& cache_prob, double match_prob) = 0;

 private:
  String ConstructReordering(const NodeIter& tree_node);

  double GetMatchProb(
      const NodeIter& tree_node,
      const AlignedTree& frag,
      const NodeIter& frag_node);

  vector<NodeIter> GetFrontierVariables(
      const NodeIter& tree_node,
      const AlignedTree& frag,
      const NodeIter& frag_node);

  shared_ptr<pair<Rule, double>> SelectRule(const NodeIter& node);

  virtual shared_ptr<pair<Rule, double>> SelectRule(
      const vector<pair<Rule, double>>& candidates) = 0;

  static const double FAIL;
  static const double STOP;

  AlignedTree tree;
  Grammar grammar;
  shared_ptr<RuleStatsReporter> reporter;
  Cache cache;
};

#endif
