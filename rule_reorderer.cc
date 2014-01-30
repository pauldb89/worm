#include "rule_reorderer.h"

#include <unordered_map>

RuleReorderer::RuleReorderer(
    double penalty, int max_leaves, int max_tree_size) :
    penalty(penalty), max_leaves(max_leaves), max_tree_size(max_tree_size) {}

String RuleReorderer::Reorder(
    const AlignedTree& tree, const Alignment& alignment) const {
  vector<NodeIter> source_items;
  for (auto leaf = tree.begin_leaf(); leaf != tree.end_leaf(); ++leaf) {
    source_items.push_back(leaf);
  }

  int source_size = source_items.size();
  int tree_size = tree.size();
  if (source_size > max_leaves || tree_size > max_tree_size) {
    vector<int> permutation;
    for (int i = 0; i < source_size; ++i) {
      permutation.push_back(i);
    }
    return ConstructRuleReordering(source_items, permutation);
  }

  unordered_map<int, vector<int>> forward_links;
  for (auto link: alignment) {
    forward_links[link.first].push_back(link.second);
  }

  vector<vector<int>> crossing_alignments(source_size);
  for (int i = 0; i < source_size; ++i) {
    crossing_alignments[i].resize(source_size);
    for (int j = 0; j < source_size; ++j) {
      if (i != j) {
        for (int x: forward_links[i]) {
          for (int y: forward_links[j]) {
            crossing_alignments[i][j] += x > y;
          }
        }
      }
    }
  }

  vector<double> min_cost(1 << source_size);
  vector<int> last_bit(1 << source_size);
  for (size_t state = 1; state < min_cost.size(); ++state) {
    min_cost[state] = numeric_limits<double>::infinity();
    for (int i = 0; i < source_size; ++i) {
      if (state & (1 << i)) {
        int other_bits = 0;
        double cost = min_cost[state ^ (1 << i)];
        for (int j = 0; j < source_size; ++j) {
          if (i != j && (state & (1 << j))) {
            ++other_bits;
            cost += crossing_alignments[j][i];
          }
        }
        cost += penalty * abs(other_bits - i);

        if (cost < min_cost[state]) {
          min_cost[state] = cost;
          last_bit[state] = i;
        }
      }
    }
  }

  int state = (1 << source_size) - 1;
  vector<int> permutation(source_size);
  for (int i = source_size - 1; i >= 0; --i) {
    permutation[i] = last_bit[state];
    state ^= 1 << last_bit[state];
  }

  return ConstructRuleReordering(source_items, permutation);
}

String RuleReorderer::ConstructRuleReordering(
    const vector<NodeIter>& source_items,
    const vector<int>& permutation) const {
  String reordering;
  int word_index = 0;
  for (auto index: permutation) {
    auto item = source_items[index];
    if (item->IsSetWord()) {
      reordering.push_back(StringNode(item->GetWord(), word_index, -1));
      ++word_index;
    } else {
      int var_index = 0;
      for (int i = 0; i < index; ++i) {
        var_index += !source_items[i]->IsSetWord();
      }
      reordering.push_back(StringNode(-1, -1, var_index));
    }
  }

  return reordering;
}
