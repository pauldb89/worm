#include "viterbi_reorderer.h"

#include <iostream>

#include "dictionary.h"
#include "grammar.h"

const double ViterbiReorderer::FAIL = -numeric_limits<double>::infinity();
const double ViterbiReorderer::STOP = -1e6;

ViterbiReorderer::ViterbiReorderer(shared_ptr<Grammar> grammar,
                                   Dictionary& dictionary) :
    grammar(grammar), dictionary(dictionary), total_nodes(0), skipped_nodes(0) {}

String ViterbiReorderer::Reorder(const AlignedTree& tree, int sentence_index) {
  total_nodes += tree.size();

  map<NodeIter, double> cache;
  map<NodeIter, Rule> best_rules;
  for (auto node = tree.begin_post(); node != tree.end_post(); ++node) {
    vector<pair<Rule, double>> rules = grammar->GetRules(node->GetTag());

    cache[node] = STOP * tree.size(node);
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

  String result = ConstructReordering(best_rules, tree, tree.begin(), sentence_index);
  for (size_t i = 0; i < result.size(); ++i) {
    result[i].SetWordIndex(i);
  }
  return result;
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
    NodeIter tree_node, int sentence_index) {
  String result;
  // If the subtree was impossible to parse, assume the children of the current
  // node do not need to be reordered.
  if (best_rules.count(tree_node) == 0) {
    skipped_nodes += tree_node.number_of_children();

    if (tree_node.number_of_children() == 0) {
      result.push_back(StringNode(tree_node->GetWord(), -1, -1));
    } else {
      for (auto child = tree.begin(tree_node);
           child != tree.end(tree_node);
           ++child) {
        String subresult = ConstructReordering(best_rules, tree, child, sentence_index);
        copy(subresult.begin(), subresult.end(), back_inserter(result));
      }
    }
    return result;
  }

  const Rule& rule = best_rules.at(tree_node);
  grammar->UpdateRuleStats(rule, sentence_index);
  const AlignedTree& frag = rule.first;
  vector<NodeIter> frontier_variables = GetFrontierVariables(
      tree, tree_node, frag, frag.begin());

  const String& reordering = rule.second;
  for (auto node: reordering) {
    if (node.IsSetWord()) {
      result.push_back(node);
    } else {
      auto subresult = ConstructReordering(
          best_rules, tree, frontier_variables[node.GetVarIndex()], sentence_index);
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

double ViterbiReorderer::GetSkippedNodesRatio() {
  return (double) skipped_nodes / total_nodes;
}
