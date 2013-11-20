#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_

#include <fstream>
#include <map>
#include <unordered_map>
#include <vector>

#include "aligned_tree.h"
#include "rule_reorderer.h"
#include "util.h"

using namespace std;

class Dictionary;

class Grammar {
 public:
  Grammar(ifstream& grammar_stream, ifstream& alignment_stream,
          Dictionary& dictionary, double penalty, double threshold,
          int max_leaves, int max_tree_size);

  vector<pair<Rule, double>> GetRules(int tag) const;

 private:
  // Removes nonterminal-terminal and terminal-nonterminal links from the
  // alignment. These links may appear due to symmetrization.
  void RemoveMixedLinks(const Rule& rule, Alignment& alignment);

  String FindBestReordering(const AlignedTree& tree,
                            const Alignment& alignment);

  String ConstructRuleReordering(const vector<NodeIter>& source_items,
                                 const vector<int>& permutation);

  RuleReorderer rule_reorderer;
  unordered_map<int, vector<pair<Rule, double>>> rules;
};

#endif
