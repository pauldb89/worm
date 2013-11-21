#include "reorderer.h"

#include <iostream>

#include "grammar.h"
#include "rule_stats_reporter.h"

const double Reorderer::FAIL = -numeric_limits<double>::infinity();
const double Reorderer::STOP = -1e6;

Reorderer::Reorderer(
    const AlignedTree& tree,
    const Grammar& grammar,
    shared_ptr<RuleStatsReporter> reporter) :
    tree(tree), matcher(grammar, this->tree), reporter(reporter) {}

void Reorderer::ConstructProbabilityCache() {
  for (auto node = tree.begin_post(); node != tree.end_post(); ++node) {
    cache[node] = STOP * tree.size(node);
    for (const auto& match: matcher.GetRules(node)) {
      double match_prob = match.first.second;
      for (const auto& frontier_node: match.second) {
        match_prob += cache[frontier_node];
      }
      Combine(cache[node], match_prob);
    }
  }
}

String Reorderer::ConstructReordering() {
  String reordering = ConstructReordering(tree.begin());
  for (size_t i = 0; i < reordering.size(); ++i) {
    reordering[i].SetWordIndex(i);
  }
  return reordering;
}

String Reorderer::ConstructReordering(const NodeIter& tree_node) {
  String result;
  shared_ptr<pair<Rule, double>> rule = SelectRule(tree_node);
  if (rule == nullptr) {
    if (tree_node.number_of_children() == 0) {
      // Unknown terminal: Not much to do about it, simply return it as is.
      result.push_back(StringNode(tree_node->GetWord(), -1, -1));
    } else {
      // Unknown interior rule: No reordering is applied.
      auto child = tree.begin(tree_node);
      while (child != tree.end(tree_node)) {
        String subresult = ConstructReordering(child);
        copy(subresult.begin(), subresult.end(), back_inserter(result));
        ++child;
      }
    }
    return result;
  }

  reporter->UpdateRuleStats(*rule);

  vector<NodeIter> frontier;
  bool match_frontier = false;
  for (const auto& match: matcher.GetRules(tree_node)) {
    if (match.first.first == rule->first) {
      frontier = match.second;
      match_frontier = true;
      break;
    }
  }
  assert(match_frontier);
  const String& reordered_frontier = rule->first.second;
  for (const auto& node: reordered_frontier) {
    if (node.IsSetWord()) {
      result.push_back(node);
    } else {
      const auto& subresult = ConstructReordering(frontier[node.GetVarIndex()]);
      copy(subresult.begin(), subresult.end(), back_inserter(result));
    }
  }
  return result;
}

shared_ptr<pair<Rule, double>> Reorderer::SelectRule(const NodeIter& node) {
  vector<pair<Rule, double>> candidates;
  for (const auto& match: matcher.GetRules(node)) {
    double match_prob = match.first.second;
    for (const auto& frontier_node: match.second) {
      match_prob += cache[frontier_node];
    }
    candidates.push_back(make_pair(match.first.first, match_prob));
  }

  return SelectRule(candidates);
}
