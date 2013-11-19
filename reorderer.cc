#include "reorderer.h"

#include <iostream>
#include <typeinfo>

#include "grammar.h"
#include "rule_stats_reporter.h"

const double Reorderer::FAIL = -numeric_limits<double>::infinity();
const double Reorderer::STOP = -1e6;

Reorderer::Reorderer(
    const AlignedTree& tree,
    const Grammar& grammar,
    shared_ptr<RuleStatsReporter> reporter) :
    tree(tree), grammar(grammar), reporter(reporter) {
  this->grammar.Filter(tree);
}

void Reorderer::ConstructProbabilityCache() {
  for (auto node = tree.begin_post(); node != tree.end_post(); ++node) {
    cache[node] = STOP * tree.size(node);
    for (pair<Rule, double> rule: grammar.GetRules(node->GetTag())) {
      const AlignedTree& frag = rule.first.first;
      double rule_prob = log(rule.second);
      double match_prob = GetMatchProb(node, frag, frag.begin());
      Combine(cache[node], match_prob + rule_prob);
    }
  }
}

double Reorderer::GetMatchProb(
    const NodeIter& tree_node,
    const AlignedTree& frag,
    const NodeIter& frag_node) {
  if (tree_node->GetTag() != frag_node->GetTag()) {
    return FAIL;
  }

  if (frag_node.number_of_children() == 0) {
    if (frag_node->IsSetWord()) {
      return frag_node->GetWord() == tree_node->GetWord() ? 0 : FAIL;
    } else {
      return cache.at(tree_node);
    }
  }

  if (tree_node.number_of_children() != frag_node.number_of_children()) {
    return FAIL;
  }

  double match_prob = 0;
  auto tree_child = tree.begin(tree_node), frag_child = frag.begin(frag_node);
  while (tree_child != tree.end(tree_node) &&
         frag_child != frag.end(frag_node)) {
    match_prob += GetMatchProb(tree_child, frag, frag_child);
    ++tree_child; ++frag_child;
  }

  return match_prob;
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
  const AlignedTree& frag = rule->first.first;
  const vector<NodeIter>& frontier_variables =
      GetFrontierVariables(tree_node, frag, frag.begin());
  const String& reordering = rule->first.second;
  for (const auto& node: reordering) {
    if (node.IsSetWord()) {
      result.push_back(node);
    } else {
      auto subresult = ConstructReordering(
          frontier_variables[node.GetVarIndex()]);
      copy(subresult.begin(), subresult.end(), back_inserter(result));
    }
  }
  return result;
}

shared_ptr<pair<Rule, double>> Reorderer::SelectRule(const NodeIter& node) {
  vector<pair<Rule, double>> candidates;
  for (const auto& rule: grammar.GetRules(node->GetTag())) {
    const AlignedTree& frag = rule.first.first;
    double rule_prob = log(rule.second);
    double match_prob = GetMatchProb(node, frag, frag.begin());
    if (match_prob != FAIL) {
      candidates.push_back(make_pair(rule.first, match_prob + rule_prob));
    }
  }

  return SelectRule(candidates);
}

vector<NodeIter> Reorderer::GetFrontierVariables(
    const NodeIter& tree_node,
    const AlignedTree& frag,
    const NodeIter& frag_node) {
  vector<NodeIter> frontier_variables;
  if (frag_node.number_of_children() == 0) {
    if (!frag_node->IsSetWord()) {
      frontier_variables.push_back(tree_node);
    }
    return frontier_variables;
  }

  auto tree_child = tree.begin(tree_node), frag_child = frag.begin(frag_node);
  while (tree_child != tree.end(tree_node) &&
         frag_child != frag.end(frag_node)) {
    auto variables = GetFrontierVariables(tree_child, frag, frag_child);
    copy(variables.begin(), variables.end(), back_inserter(frontier_variables));
    ++tree_child; ++frag_child;
  }
  return frontier_variables;
}
