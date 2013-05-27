#ifndef _VITERBI_REORDERER_H_
#define _VITERBI_REORDERER_H_

#include "grammar.h"

class ViterbiReorderer {
 public:
  ViterbiReorderer(const Grammar& grammar);

  String Reorder(const AlignedTree& tree);

 private:
  Grammar grammar;
};

#endif
