#include "grammar.h"

#include <string>
#include <unordered_set>

#include <boost/regex.hpp>

#include "dictionary.h"

Grammar::Grammar(ifstream& grammar_stream, ifstream& alignment_stream,
                 Dictionary& dictionary, double penalty,
                 int max_leaves, int max_tree_size) :
    penalty(penalty), max_leaves(max_leaves), max_tree_size(max_tree_size) {
  map<Rule, double> reordering_probs;
  while (!grammar_stream.eof()) {
    pair<Rule, double> entry = ReadRule(grammar_stream, dictionary);
    Alignment alignment;
    alignment_stream >> alignment;

    const Rule& rule = entry.first;
    RemoveMixedLinks(rule, alignment);

    double prob = entry.second;
    const AlignedTree& tree = rule.first;
    String reordering = FindBestReordering(tree, alignment);
    reordering_probs[make_pair(tree, reordering)] += prob;

    grammar_stream >> ws;
  }

  for (auto rule: reordering_probs) {
    rules[rule.first.first.GetRootTag()].push_back(rule);
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

String Grammar::FindBestReordering(const AlignedTree& tree,
                                   const Alignment& alignment) {
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
    return ConstructReordering(source_items, permutation);
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
          if (i != j && state & (1 << j)) {
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

  return ConstructReordering(source_items, permutation);
}

String Grammar::ConstructReordering(const vector<NodeIter>& source_items,
                                    const vector<int>& permutation) {
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

vector<pair<Rule, double>> Grammar::GetRules(int root_tag) {
  if (!rules.count(root_tag)) {
    return vector<pair<Rule, double>>();
  }
  return rules[root_tag];
}

void Grammar::UpdateRuleStats(const Rule& rule) {
  const AlignedTree& tree = rule.first;
  const String& reordering = rule.second;
  bool no_reordering = true;
  int target_index = 0, var_index = 0;
  for (auto leaf = tree.begin_leaf(); leaf != tree.end_leaf(); ++leaf) {
    if (leaf->IsSetWord() ^ reordering[target_index].IsSetWord()) {
      no_reordering = false;
      break;
    }

    if (leaf->IsSetWord() && reordering[target_index].IsSetWord() &&
        leaf->GetWord() != reordering[target_index].GetWord()) {
      no_reordering = false;
      break;
    }

    if (!leaf->IsSetWord() && !reordering[target_index].IsSetWord() &&
        var_index != reordering[target_index].GetVarIndex()) {
      no_reordering = false;
      break;
    }

    var_index += !leaf->IsSetWord();
    ++target_index;
  }

  if (!no_reordering) {
    #pragma omp critical
    ++rule_counts[rule];
  }
}

void Grammar::DisplayRuleStats(ostream& stream, Dictionary& dictionary) {
  double total_rules = 0;
  vector<pair<int, Rule>> top_rules;
  for (auto rule: rule_counts) {
    top_rules.push_back(make_pair(rule.second, rule.first));
    total_rules += rule.second;
  }

  sort(top_rules.begin(), top_rules.end(), greater<pair<int, Rule>>());

  double num_rules = 0;
  for (auto rule: top_rules) {
    num_rules += rule.first;
    WriteSTSGRule(stream, rule.second, dictionary);
    stream << " ||| " << rule.first << " ||| " << num_rules / total_rules
           << endl;
  }
}
