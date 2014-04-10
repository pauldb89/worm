#include "grammar.h"

#include <string>
#include <unordered_set>

#include <boost/regex.hpp>

#include "dictionary.h"

Grammar::Grammar(ifstream& grammar_stream, ifstream& alignment_stream,
                 Dictionary& dictionary, double penalty, double threshold,
                 int max_leaves, int max_tree_size) :
    rule_reorderer(penalty, max_leaves, max_tree_size) {
  map<Rule, double> reordering_probs;
  while (!grammar_stream.eof()) {
    pair<Rule, double> entry = ReadRule(grammar_stream, dictionary);
    Alignment alignment;
    alignment_stream >> alignment;

    const Rule& rule = entry.first;
    RemoveMixedLinks(rule, alignment);

    double prob = entry.second;
    const AlignedTree& tree = rule.first;
    String reordering = rule_reorderer.Reorder(tree, alignment);
    reordering_probs[make_pair(tree, reordering)] += prob;

    grammar_stream >> ws;
  }

  for (auto rule: reordering_probs) {
    if (rule.second >= threshold) {
      rules[rule.first.first.GetRootTag()].push_back(
          make_pair(rule.first, log(rule.second)));
    }
  }
}


void Grammar::RemoveMixedLinks(const Rule& rule, Alignment& alignment) {
  const AlignedTree& tree = rule.first;
  unordered_set<int> source_var_indexes;
  int leaf_index = 0;
  for (auto leaf = tree.begin_leaf(); leaf != tree.end_leaf(); ++leaf) {
    if (leaf->IsSplitNode()) {
      source_var_indexes.insert(leaf_index);
    }
    ++leaf_index;
  }

  const String& target_string = rule.second;
  unordered_set<int> target_var_indexes;
  for (size_t i = 0; i < target_string.size(); ++i) {
    if (!target_string[i].IsSetWord()) {
      target_var_indexes.insert(i);
    }
  }

  alignment.erase(remove_if(alignment.begin(), alignment.end(),
      [&source_var_indexes, &target_var_indexes](const pair<int, int>& link) {
        return source_var_indexes.count(link.first) ^
               target_var_indexes.count(link.second);
      }), alignment.end());
}

bool Grammar::HasRules(int root_tag) const {
  return rules.count(root_tag);
}

const vector<pair<Rule, double>>& Grammar::GetRules(int root_tag) const {
  return rules.at(root_tag);
}
