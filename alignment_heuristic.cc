#include "alignment_heuristic.h"

#include <boost/functional/hash.hpp>

#include "aligned_tree.h"
#include "translation_table.h"
#include "util.h"

typedef boost::hash<pair<int, int>> PairHasher;
typedef pair<double, pair<int, size_t>> State;

AlignmentHeuristic::AlignmentHeuristic(
    shared_ptr<TranslationTable> forward_table,
    shared_ptr<TranslationTable> reverse_table,
    const unordered_set<int>& blacklisted_tags,
    size_t max_additional_links) :
    forward_table(forward_table),
    reverse_table(reverse_table),
    blacklisted_tags(blacklisted_tags),
    max_additional_links(max_additional_links) {}

Alignment AlignmentHeuristic::GetBaseAlignment(
    const AlignedTree& tree,
    const Alignment& intersect_alignment) const {
  vector<int> source_tags;
  for (auto leaf = tree.begin_leaf(); leaf != tree.end_leaf(); ++leaf) {
    source_tags.push_back(leaf->GetTag());
  }

  Alignment base_alignment;
  for (const auto& link: intersect_alignment) {
    if (!blacklisted_tags.count(source_tags[link.first])) {
      base_alignment.push_back(link);
    }
  }

  return base_alignment;
}

Alignment AlignmentHeuristic::GetAdditionalLinks(
    const AlignedTree& tree,
    const String& target_string,
    const Alignment& gdfa_alignment,
    const Alignment& base_alignment) const {
  vector<int> source_words;
  for (auto leaf = tree.begin_leaf(); leaf != tree.end_leaf(); ++leaf) {
    source_words.push_back(leaf->GetWord());
  }

  vector<int> target_words;
  for (auto node: target_string) {
    target_words.push_back(node.GetWord());
  }

  unordered_set<pair<int, int>, PairHasher> base_links;
  for (const auto& link: base_alignment) {
    base_links.insert(link);
  }

  vector<pair<double, pair<int, int>>> candidates;
  for (const auto& link: gdfa_alignment) {
    if (!base_links.count(link)) {
      double forward_prob = forward_table->GetProbability(
          source_words[link.first], target_words[link.second]);
      double reverse_prob = forward_table->GetProbability(
          target_words[link.second], source_words[link.first]);
      candidates.push_back(make_pair(forward_prob + reverse_prob, link));
    }
  }

  sort(candidates.begin(), candidates.end(),
      greater<pair<double, pair<int, int>>>());
  Alignment additional_links;
  for (size_t i = 0; i < candidates.size(); ++i) {
    additional_links.push_back(candidates[i].second);
  }

  return additional_links;
}

double AlignmentHeuristic::GetInteriorNodesRatio(
    const AlignedTree& tree) const {
  int total_rules = 0, interior_nodes = 0;
  for (auto node: tree) {
    if (node.IsSplitNode()) {
      ++total_rules;
    } else {
      ++interior_nodes;
    }
  }

  return (double) interior_nodes / total_rules;
}

Alignment AlignmentHeuristic::FindBestAlignment(
    const AlignedTree& parse_tree,
    const String& target_string,
    const Alignment& gdfa_alignment,
    const Alignment& intersect_alignment) const {
  Alignment base_alignment = GetBaseAlignment(parse_tree, intersect_alignment);
  Alignment additional_links = GetAdditionalLinks(
      parse_tree, target_string, gdfa_alignment, base_alignment);

  Alignment best_alignment = base_alignment;
  AlignedTree tree = parse_tree;
  ConstructGHKMDerivation(tree, target_string, base_alignment);
  double best_ratio = GetInteriorNodesRatio(tree);

  int num_states = 0;
  priority_queue<State, vector<State>, greater<State>> states;
  states.push(make_pair(best_ratio, make_pair(0, 0)));
  while (states.size() > 0 && num_states < MAX_STATES) {
    ++num_states;
    auto state = states.top();
    states.pop();

    double current_ratio = state.first;
    int encoding = state.second.first;
    size_t index = state.second.second;

    if (index == additional_links.size()) {
      continue;
    }

    states.push(make_pair(current_ratio, make_pair(encoding, index + 1)));

    int new_encoding = encoding | (1 << index);
    Alignment alignment = base_alignment;
    for (size_t i = 0; i < index; ++i) {
      if (encoding & (1 << i)) {
        alignment.push_back(additional_links[i]);
      }
    }

    AlignedTree tree = parse_tree;
    ConstructGHKMDerivation(tree, target_string, alignment);
    double new_ratio = GetInteriorNodesRatio(tree);
    if (new_ratio <= current_ratio) {
      states.push(make_pair(new_ratio, make_pair(new_encoding, index + 1)));
    }

    if (new_ratio < best_ratio) {
      best_ratio = new_ratio;
      best_alignment = alignment;
    }
  }

  return best_alignment;
}
