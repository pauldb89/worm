#include "rule_matcher.h"

#include <iostream>

RuleMatcher::RuleMatcher(const Grammar& grammar, const AlignedTree& tree) {
  for (auto node = tree.begin(); node != tree.end(); ++node) {
    // Hack to enable passing grammar output by reference.
    if (grammar.HasRules(node->GetTag())) {
      for (const auto& rule: grammar.GetRules(node->GetTag())) {
        vector<NodeIter> frontier;
        const AlignedTree& frag = rule.first.first;
        if (MatchRule(tree, node, frag, frag.begin(), frontier)) {
          matcher[node].push_back(make_pair(rule, frontier));
        }
      }
    }
  }
}

bool RuleMatcher::MatchRule(
    const AlignedTree& tree, const NodeIter& tree_node,
    const AlignedTree& frag, const NodeIter& frag_node,
    vector<NodeIter>& frontier) const {
  if (tree_node->GetTag() != frag_node->GetTag()) {
    return false;
  }

  if (frag_node.number_of_children() == 0) {
    if (frag_node->IsSetWord()) {
      return frag_node->GetWord() == tree_node->GetWord();
    }

    frontier.push_back(tree_node);
    return true;
  }

  if (tree_node.number_of_children() != frag_node.number_of_children()) {
    return false;
  }

  auto tree_child = tree.begin(tree_node), frag_child = frag.begin(frag_node);
  while (tree_child != tree.end(tree_node) &&
         frag_child != frag.end(frag_node)) {
    if (!MatchRule(tree, tree_child, frag, frag_child, frontier)) {
      return false;
    }
    ++tree_child, ++frag_child;
  }

  return true;
}

MatchingRules RuleMatcher::GetRules(const NodeIter& node) const {
  if (!matcher.count(node)) {
    return MatchingRules();
  }

  return matcher.at(node);
}
