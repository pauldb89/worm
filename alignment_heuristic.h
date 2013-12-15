#pragma once

#include <memory>
#include <unordered_set>

#include "definitions.h"

class TranslationTable;

using namespace std;

class AlignmentHeuristic {
 public:
  AlignmentHeuristic(
      shared_ptr<TranslationTable> forward_table,
      shared_ptr<TranslationTable> reverse_table,
      const unordered_set<int>& blacklisted_tags,
      size_t max_additional_links);

  Alignment FindBestAlignment(
      const AlignedTree& parse_tree,
      const String& target_string,
      const Alignment& gdfa_alignment,
      const Alignment& intersect_alignment) const;

 private:
  Alignment GetBaseAlignment(
      const AlignedTree& parse_tree,
      const Alignment& intersect_alignment) const;

  Alignment GetAdditionalLinks(
      const AlignedTree& parse_tree,
      const String& target_string,
      const Alignment& gdfa_alignment,
      const Alignment& base_alignment) const;

  double GetInteriorNodesRatio(const AlignedTree& tree) const;

  shared_ptr<TranslationTable> forward_table, reverse_table;
  unordered_set<int> blacklisted_tags;
  size_t max_additional_links;
};
