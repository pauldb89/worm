#ifndef _ALIGNED_TREE_H_
#define _ALIGNED_TREE_H_

#include <fstream>

#include "node.h"
#include "tree.h"
#include "util.h"

using namespace std;

class AlignedTree: public tree<AlignedNode> {
 public:
  friend Instance ReadInstance(ifstream& tree_infile,
                               ifstream& string_infile,
                               Dictionary& dictionary);

  vector<AlignedNode> GetVariables();

  iterator GetSplitAncestor(const iterator& node) const;

  AlignedTree GetFragment(const iterator& node) const;

  void ConstructFragment(const iterator& node,
                         AlignedTree& fragment,
                         const iterator& current_node) const;

  vector<iterator> GetSplitDescendants(const iterator& root) const;

  bool operator<(const AlignedTree& tree) const;

  bool operator==(const AlignedTree& tree) const;
};

#endif
