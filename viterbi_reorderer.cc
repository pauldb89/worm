#include "viterbi_reorderer.h"

#include <iostream>

// TODO(pauldb): Remove dictionary when done.
#include "dictionary.h"

const double ViterbiReorderer::FAIL = -numeric_limits<double>::infinity();

ViterbiReorderer::ViterbiReorderer(const Grammar& grammar) :
    grammar(grammar) {}

String ViterbiReorderer::Reorder(const AlignedTree& tree, Dictionary& dictionary) {
  map<NodeIter, double> cache;
  map<NodeIter, Rule> best_rules;
  for (auto node = tree.begin_post(); node != tree.end_post(); ++node) {
    vector<pair<Rule, double>> rules = grammar.GetRules(node->GetTag());

    cache[node] = FAIL;
    for (auto entry: rules) {
      const Rule& rule = entry.first;
      const AlignedTree& frag = rule.first;
      double prob = ComputeProbability(cache, tree, node, frag, frag.begin()) +
                    log(entry.second);
      if (prob > cache[node]) {
        cache[node] = prob;
        best_rules[node] = rule;
      }
    }
  }

  if (cache[tree.begin()] == FAIL) {
    return String();
  }

  return ConstructReordering(best_rules, tree, tree.begin());
}

double ViterbiReorderer::ComputeProbability(
    const map<NodeIter, double>& cache, const AlignedTree& tree,
    NodeIter tree_node, const AlignedTree& frag, NodeIter frag_node) {
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

  double result = 0;
  auto tree_child = tree.begin(tree_node), frag_child = frag.begin(frag_node);
  while (tree_child != tree.end(tree_node) &&
         frag_child != frag.end(frag_node)) {
    result += ComputeProbability(cache, tree, tree_child, frag, frag_child);
    ++tree_child; ++frag_child;
  }

  return result;
}

String ViterbiReorderer::ConstructReordering(
    const map<NodeIter, Rule>& best_rules, const AlignedTree& tree,
    NodeIter tree_node) {
  const Rule& rule = best_rules.at(tree_node);
  const AlignedTree& frag = rule.first;
  vector<NodeIter> frontier_variables = GetFrontierVariables(
      tree, tree_node, frag, frag.begin());

  String result;
  const String& reordering = rule.second;
  for (auto node: reordering) {
    if (node.IsSetWord()) {
      result.push_back(node);
    } else {
      auto subresult = ConstructReordering(
          best_rules, tree, frontier_variables[node.GetVarIndex()]);
      copy(subresult.begin(), subresult.end(), back_inserter(result));
    }
  }

  return result;
}

vector<NodeIter> ViterbiReorderer::GetFrontierVariables(
    const AlignedTree& tree, NodeIter tree_node,
    const AlignedTree& frag, NodeIter frag_node) {
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
    auto variables = GetFrontierVariables(tree, tree_child, frag, frag_child);
    copy(variables.begin(), variables.end(), back_inserter(frontier_variables));
    ++tree_child; ++frag_child;
  }
  return frontier_variables;
}
