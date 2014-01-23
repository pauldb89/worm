#include "alignment_constructor.h"

#include "dictionary.h"
#include "translation_table.h"

AlignmentConstructor::AlignmentConstructor(
    shared_ptr<TranslationTable> forward_table,
    shared_ptr<TranslationTable> reverse_table) :
    forward_table(forward_table), reverse_table(reverse_table) {}

pair<Alignment, Alignment> AlignmentConstructor::ExtractAlignments(
    const Instance& instance) {
  Alignment forward_alignment, reverse_alignment;
  const AlignedTree& tree = instance.first;
  for (auto node = tree.begin(); node != tree.end(); ++node) {
    if (node->IsSplitNode()) {
      const Rule& rule = extractor.ExtractRule(instance, node);
      const AlignedTree& frag = rule.first;
      const String& target_string = rule.second;

      auto subalignments = ConstructTerminalLinks(rule);

      vector<NodeIter> leaves;
      for (auto leaf = frag.begin_leaf(); leaf != frag.end_leaf(); ++leaf) {
        leaves.push_back(leaf);
      }

      for (auto link: subalignments.first) {
        int source_index = leaves[link.first]->GetWordIndex();
        int target_index = target_string[link.second].GetWordIndex();
        forward_alignment.push_back(make_pair(source_index, target_index));
      }

      for (auto link: subalignments.second) {
        int source_index = leaves[link.first]->GetWordIndex();
        int target_index = target_string[link.second].GetWordIndex();
        reverse_alignment.push_back(make_pair(source_index, target_index));
      }
    }
  }

  return make_pair(forward_alignment, reverse_alignment);
}

pair<Alignment, Alignment> AlignmentConstructor::ConstructAlignments(
    const Rule& rule) {
  Alignment forward_alignment, reverse_alignment;

  auto nonterminal_links = ConstructNonterminalLinks(rule);
  copy(nonterminal_links.begin(), nonterminal_links.end(),
       back_inserter(forward_alignment));
  copy(nonterminal_links.begin(), nonterminal_links.end(),
       back_inserter(reverse_alignment));

  auto terminal_links = ConstructTerminalLinks(rule);
  copy(terminal_links.first.begin(), terminal_links.first.end(),
       back_inserter(forward_alignment));
  copy(terminal_links.second.begin(), terminal_links.second.end(),
       back_inserter(reverse_alignment));

  return make_pair(forward_alignment, reverse_alignment);
}

Alignment AlignmentConstructor::ConstructNonterminalLinks(const Rule& rule) {
  const AlignedTree& frag = rule.first;
  const String& target_string = rule.second;

  Alignment alignment;
  int leaf_index = 0, var_index = 0;
  for (auto leaf = frag.begin_leaf(); leaf != frag.end_leaf(); ++leaf) {
    if (leaf->IsSplitNode() && leaf != frag.begin()) {
      for (size_t i = 0; i < target_string.size(); ++i) {
        if (target_string[i].GetVarIndex() == var_index) {
          alignment.push_back(make_pair(leaf_index, i));
          break;
        }
      }
      ++var_index;
    }
    ++leaf_index;
  }

  return alignment;
}

pair<Alignment, Alignment> AlignmentConstructor::ConstructTerminalLinks(
    const Rule& rule) {
  const AlignedTree& frag = rule.first;
  const String& target_string = rule.second;

  Alignment forward_alignment;
  for (size_t i = 0; i < target_string.size(); ++i) {
    if (!target_string[i].IsSetWord()) {
      continue;
    }

    int target_word = target_string[i].GetWord(), leaf_index = 0;
    double best_match = forward_table->GetProbability(
        Dictionary::NULL_WORD_ID, target_word);
    int best_index = -1;
    for (auto leaf = frag.begin_leaf(); leaf != frag.end_leaf(); ++leaf) {
      if (leaf->IsSetWord() && (!leaf->IsSplitNode() || leaf == frag.begin())) {
        double match_prob = forward_table->GetProbability(
            leaf->GetWord(), target_word);
        if (match_prob > best_match) {
          best_match = match_prob;
          best_index = leaf_index;
        }
      }

      ++leaf_index;
    }

    if (best_index >= 0) {
      forward_alignment.push_back(make_pair(best_index, i));
    }
  }

  Alignment reverse_alignment;
  int leaf_index = 0;
  for (auto leaf = frag.begin_leaf(); leaf != frag.end_leaf(); ++leaf) {
    if (leaf->IsSetWord() && (!leaf->IsSplitNode() || leaf == frag.begin())) {
      int source_word = leaf->GetWord();
      double best_match = reverse_table->GetProbability(
          Dictionary::NULL_WORD_ID, source_word);
      int best_index = -1;
      for (size_t i = 0; i < target_string.size(); ++i) {
        if (!target_string[i].IsSetWord()) {
          continue;
        }

        double match_prob = reverse_table->GetProbability(
            target_string[i].GetWord(), source_word);
        if (match_prob > best_match) {
          best_match = match_prob;
          best_index = i;
        }
      }

      if (best_index >= 0) {
        reverse_alignment.push_back(make_pair(leaf_index, best_index));
      }
    }
    ++leaf_index;
  }

  return make_pair(forward_alignment, reverse_alignment);
}
