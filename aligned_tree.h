#ifndef _ALIGNED_TREE_H_
#define _ALIGNED_TREE_H_

#include <fstream>

#include "node.h"
#include "tree.h"

using namespace std;

class Dictionary;

class AlignedTree: public tree<AlignedNode> {
 public:
  int GetRootTag() const;

  iterator GetSplitAncestor(const iterator& node) const;

  AlignedTree GetFragment(const iterator& node) const;

  void ConstructFragment(const iterator& node,
                         AlignedTree& fragment,
                         const iterator& current_node) const;

  vector<iterator> GetSplitDescendants(const iterator& root) const;

  void DisplayTree(Dictionary& dictionary) const;

  void Write(ofstream& out, const iterator& root,
             Dictionary& dictionary, int& var_index) const;

  bool operator<(const AlignedTree& tree) const;

  bool operator==(const AlignedTree& tree) const;
};

bool operator<(const AlignedTree::iterator& it1,
               const AlignedTree::iterator& it2);

#endif
