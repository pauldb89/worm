#include "reorderer.h"

#include <iostream>

#include "grammar.h"
#include "log_add.h"
#include "rule_stats_reporter.h"

const double Reorderer::NO_MATCH = -1e3;

Reorderer::Reorderer(
    const AlignedTree& tree,
    const Grammar& grammar,
    shared_ptr<RuleStatsReporter> reporter) :
    tree(tree), matcher(grammar, this->tree), reporter(reporter) {}

void Reorderer::ConstructProbabilityCache() {
  for (auto node = tree.begin_post(); node != tree.end_post(); ++node) {
    cache[node] = Log<double>::zero();
    auto rule_matchings = matcher.GetRules(node);
    if (rule_matchings.size() > 0) {
      for (const auto& match: rule_matchings) {
        double match_prob = match.first.second;
        for (const auto& frontier_node: match.second) {
          match_prob += cache[frontier_node];
        }
        Combine(cache[node], match_prob);
      }
    } else {
      double match_prob = NO_MATCH;
      for (auto child = tree.begin(node); child != tree.end(node); ++child) {
        match_prob += cache[child];
      }
      Combine(cache[node], match_prob);
    }
  }
}

String Reorderer::ConstructReordering() {
  String reordering;
  ConstructReordering(tree.begin(), reordering);
  for (size_t i = 0; i < reordering.size(); ++i) {
    reordering[i].SetWordIndex(i);
  }
  return reordering;
}

void Reorderer::ConstructReordering(
    const NodeIter& tree_node, String& reordering) {
  shared_ptr<pair<Rule, double>> rule = SelectRule(tree_node);
  if (rule == nullptr) {
    if (tree_node.number_of_children() == 0) {
      // Unknown terminal: Not much to do about it, simply return it as is.
      reordering.push_back(StringNode(tree_node->GetWord(), -1, -1));
    } else {
      // Unknown interior rule: No reordering is applied.
      auto child = tree.begin(tree_node);
      while (child != tree.end(tree_node)) {
        ConstructReordering(child, reordering);
        ++child;
      }
    }
    return;
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
      reordering.push_back(node);
    } else {
      ConstructReordering(frontier[node.GetVarIndex()], reordering);
    }
  }
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
