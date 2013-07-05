#ifndef _REORDERER_H_
#define _REORDERER_H_

#include <map>
#include <memory>

#include "reorderer_base.h"

using namespace std;

class Grammar;

typedef AlignedTree::iterator NodeIter;
typedef map<NodeIter, double> Cache;

class Reorderer : public ReordererBase {
 public:
  Reorderer(shared_ptr<Grammar> grammar);

  String Reorder(const AlignedTree& tree);

 private:
  Cache ConstructProbabilityCache(const AlignedTree& tree);

  virtual void Combine(double& cache_prob, double match_prob) = 0;

  String ConstructReordering(const Cache& cache, const AlignedTree& tree);

  String ConstructReordering(const Cache& cache, const AlignedTree& tree,
                             const NodeIter& tree_node);

  double GetMatchProb(const Cache& cache, const AlignedTree& tree,
                      const NodeIter& tree_node, const AlignedTree& frag,
                      const NodeIter& frag_node);

  vector<NodeIter> GetFrontierVariables(
      const AlignedTree& tree, const NodeIter& tree_node,
      const AlignedTree& frag, const NodeIter& frag_node);

  shared_ptr<Rule> SelectRule(const Cache& cache, const AlignedTree& tree,
                              const NodeIter& node);

  virtual shared_ptr<Rule> SelectRule(
      const vector<pair<Rule, double>>& candidates) = 0;

  static const double FAIL;
  static const double STOP;
  shared_ptr<Grammar> grammar;
};

#endif
