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

  vector<iterator> GetSplitDescendants(const iterator& root) const;

  void Write(ostream& out, Dictionary& dictionary) const;

  bool operator<(const AlignedTree& tree) const;

  bool operator==(const AlignedTree& tree) const;

 private:
  void ConstructFragment(const iterator& node,
                         AlignedTree& fragment,
                         const iterator& current_node) const;

  void Write(ostream& out, const iterator& root,
             Dictionary& dictionary, int& var_index) const;
};

typedef AlignedTree::iterator NodeIter;

bool operator<(const NodeIter& it1, const NodeIter& it2);

#endif
