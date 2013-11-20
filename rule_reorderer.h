#pragma once

#include "aligned_tree.h"
#include "util.h"

class RuleReorderer {
 public:
  RuleReorderer(double penalty, int max_leaves, int max_tree_size);

  String Reorder(const AlignedTree& tree, const Alignment& alignment) const;

 private:
  String ConstructRuleReordering(
      const vector<NodeIter>& source_items,
      const vector<int>& permutation) const;

  double penalty;
  int max_leaves;
  int max_tree_size;
};
