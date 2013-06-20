#ifndef _VITERBI_REORDERER_H_
#define _VITERBI_REORDERER_H_

#include <map>
#include <memory>
#include <vector>

#include "aligned_tree.h"
#include "util.h"

using namespace std;

class Dictionary;
class Grammar;

typedef AlignedTree::iterator NodeIter;

class ViterbiReorderer {
 public:
  ViterbiReorderer(shared_ptr<Grammar> grammar, Dictionary& dictionary);

  String Reorder(const AlignedTree& tree, int sentence_index);

  double GetSkippedNodesRatio();

 private:
  double ComputeProbability(const map<NodeIter, double>& cache,
                            const AlignedTree& tree, NodeIter tree_node,
                            const AlignedTree& frag, NodeIter frag_node);

  String ConstructReordering(const map<NodeIter, Rule>& best_rules,
                             const AlignedTree& tree, NodeIter tree_node,
                             int sentence_index);

  vector<NodeIter> GetFrontierVariables(
      const AlignedTree& tree, NodeIter tree_node,
      const AlignedTree& frag, NodeIter frag_node);

  const static double FAIL;
  const static double STOP;

  shared_ptr<Grammar> grammar;
  Dictionary& dictionary;
  int total_nodes, skipped_nodes;
};

#endif
