#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_

#include <fstream>
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
          Dictionary& dictionary, double penalty);

 private:
  // Removes nonterminal-terminal and terminal-nonterminal links from the
  // alignment. These links may appear due to symmetrization.
  void RemoveMixedLinks(const Rule& rule, Alignment& alignment);

  String FindBestReordering(const AlignedTree& tree,
                            const Alignment& alignment);

  String ConstructReordering(const vector<NodeIter>& source_items,
                             const vector<int>& permutation);

  const static int MAX_REORDER = 15;

  double penalty;
  unordered_map<int, vector<pair<Rule, double>>> reordering_grammar;
};

#endif
