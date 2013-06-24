#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_

#include <fstream>
#include <map>
#include <unordered_map>
#include <vector>

#include "aligned_tree.h"
#include "util.h"

using namespace std;

typedef AlignedTree::iterator NodeIter;

class Dictionary;

class Grammar {
 public:
  Grammar(ifstream& grammar_stream, ifstream& alignment_stream,
          Dictionary& dictionary, double penalty,
          int max_leaves, int max_tree_size);

  Grammar(ifstream& grammar_stream, Dictionary& dictionary,
          double penalty, int max_leaves, int max_tree_size);

  vector<pair<Rule, double>> GetRules(int tag);

  void UpdateRuleStats(const Rule& rule);

  void DisplayRuleStats(ostream& stream, Dictionary& dictionary,
                        int num_sentences);

 private:
  // Removes nonterminal-terminal and terminal-nonterminal links from the
  // alignment. These links may appear due to symmetrization.
  void RemoveMixedLinks(const Rule& rule, Alignment& alignment);

  String FindBestReordering(const AlignedTree& tree,
                            const Alignment& alignment);

  String ConstructReordering(const vector<NodeIter>& source_items,
                             const vector<int>& permutation);

  double penalty;
  int max_leaves, max_tree_size;
  unordered_map<int, vector<pair<Rule, double>>> rules;
  map<Rule, double> reordering_probs;
  map<Rule, int> rule_counts;
};

#endif
