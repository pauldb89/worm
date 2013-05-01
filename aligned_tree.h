#ifndef _ALIGNED_TREE_H_
#define _ALIGNED_TREE_H_

#include <fstream>

using namespace std;

#include "node.h"
#include "tree.h"
#include "util.h"

class AlignedTree: public tree<AlignedNode> {
 public:
  friend Instance ReadInstance(ifstream& tree_infile,
                               ifstream& string_infile,
                               Dictionary& dictionary);
};

#endif
