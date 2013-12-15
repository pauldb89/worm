#include "alignment_heuristic.h"

#include <unordered_set>

#include <boost/functional/hash.hpp>

#include "aligned_tree.h"
#include "translation_table.h"
#include "util.h"

typedef boost::hash<pair<int, int>> PairHasher;

AlignmentHeuristic::AlignmentHeuristic(
    shared_ptr<TranslationTable> forward_table,
    shared_ptr<TranslationTable> reverse_table,
    size_t max_additional_links) :
    forward_table(forward_table),
    reverse_table(reverse_table),
    max_additional_links(max_additional_links) {}

Alignment AlignmentHeuristic::FindBestAlignment(
    const AlignedTree& parse_tree,
    const String& target_string,
    const Alignment& gdfa_alignment,
    const Alignment& intersect_alignment) const {
  Alignment additional_links = GetAdditionalLinks(
      parse_tree, target_string, gdfa_alignment, intersect_alignment);

  Alignment best_alignment = intersect_alignment;
  AlignedTree tree = parse_tree;
  ConstructGHKMDerivation(tree, target_string, intersect_alignment);
  double best_ratio = GetInteriorNodesRatio(tree);

  for (int i = 1; i < 1 << additional_links.size(); ++i) {
    Alignment alignment = intersect_alignment;
    for (size_t j = 0; j < additional_links.size(); ++j) {
      if (i & (1 << j)) {
        alignment.push_back(additional_links[j]);
      }
    }

    AlignedTree tree = parse_tree;
    ConstructGHKMDerivation(tree, target_string, alignment);
    double ratio = GetInteriorNodesRatio(tree);
    if (ratio < best_ratio) {
      best_alignment = alignment;
      best_ratio = ratio;
    }
  }

  return best_alignment;
}

Alignment AlignmentHeuristic::GetAdditionalLinks(
    const AlignedTree& parse_tree,
    const String& target_string,
    const Alignment& gdfa_alignment,
    const Alignment& intersect_alignment) const {
  vector<int> source_words;
  for (auto leaf = parse_tree.begin_leaf();
       leaf != parse_tree.end_leaf();
       ++leaf) {
    source_words.push_back(leaf->GetWord());
  }

  vector<int> target_words;
  for (auto node: target_string) {
    target_words.push_back(node.GetWord());
  }

  unordered_set<pair<int, int>, PairHasher> intersect_links;
  for (const auto& link: intersect_alignment) {
    intersect_links.insert(link);
  }

  vector<pair<double, pair<int, int>>> candidates;
  for (const auto& link: gdfa_alignment) {
    if (!intersect_links.count(link)) {
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
  for (size_t i = 0; i < min(candidates.size(), max_additional_links); ++i) {
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

