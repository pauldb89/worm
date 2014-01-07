#include "rule_extractor.h"

Rule RuleExtractor::ExtractRule(
    const Instance& instance, const NodeIter& node) const {
  AlignedTree fragment = instance.first.GetFragment(node);
  String target_string = ConstructRuleTargetSide(fragment, instance.second);
  return make_pair(fragment, target_string);
}

String RuleExtractor::ConstructRuleTargetSide(
    const AlignedTree& fragment, const String& target_string) const {
  pair<int, int> root_span = fragment.begin()->GetSpan();
  vector<int> frontier(root_span.second, -1);
  int num_split_leaves = 0;
  for (auto leaf = fragment.begin_leaf(); leaf != fragment.end_leaf(); ++leaf) {
    if (leaf != fragment.begin() && leaf->IsSplitNode()) {
      pair<int, int> span = leaf->GetSpan();
      for (int j = span.first; j < span.second; ++j) {
        frontier[j] = num_split_leaves;
      }
      ++num_split_leaves;
    }
  }

  String result;
  for (int i = root_span.first; i < root_span.second; ++i) {
    if (frontier[i] == -1) {
      result.push_back(target_string[i]);
    } else if (result.empty() || result.back().GetVarIndex() != frontier[i]) {
      result.push_back(StringNode(-1, -1, frontier[i]));
    }
  }

  return result;
}
