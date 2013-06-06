#ifndef _VITERBI_REORDERER_H_
#define _VITERBI_REORDERER_H_

#include <map>

#include "grammar.h"

using namespace std;

class ViterbiReorderer {
 public:
  ViterbiReorderer(const Grammar& grammar, Dictionary& dictionary);

  // TODO(pauldb): Remove dictionary when done.
  String Reorder(const AlignedTree& tree);

  double GetSkippedNodesRatio();

 private:
  double ComputeProbability(const map<NodeIter, double>& cache,
                            const AlignedTree& tree, NodeIter tree_node,
                            const AlignedTree& frag, NodeIter frag_node);

  String ConstructReordering(const map<NodeIter, Rule>& best_rules,
                             const AlignedTree& tree, NodeIter tree_node);

  vector<NodeIter> GetFrontierVariables(
      const AlignedTree& tree, NodeIter tree_node,
      const AlignedTree& frag, NodeIter frag_node);

  const static double FAIL;
  const static double STOP;

  Grammar grammar;
  Dictionary& dictionary;
  // TODO(pauldb): Remove when no longer necessary.
  int total_nodes, skipped_nodes;
};

#endif
