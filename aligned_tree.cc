#include "aligned_tree.h"

#include <iostream>

#include "dictionary.h"

typedef AlignedTree::iterator NodeIter;

int AlignedTree::GetRootTag() const {
  return begin()->GetTag();
}

NodeIter AlignedTree::GetSplitAncestor(const NodeIter& node) const {
  NodeIter ancestor = parent(node);
  while (!ancestor->IsSplitNode()) {
    ancestor = parent(ancestor);
  }

  return ancestor;
}

AlignedTree AlignedTree::GetFragment(const NodeIter& node) const {
  AlignedTree fragment;
  fragment.set_head(*node);
  ConstructFragment(node, fragment, fragment.begin());

  return fragment;
}

void AlignedTree::ConstructFragment(const NodeIter& node,
                                    AlignedTree& fragment,
                                    const NodeIter& fragment_node) const {
  for (sibling_iterator sibling = begin(node); sibling != end(node); ++sibling) {
    auto new_node = fragment.append_child(fragment_node, *sibling);
    if (!sibling->IsSplitNode()) {
      ConstructFragment(sibling, fragment, new_node);
    } else {
      new_node->UnsetWord();
    }
  }
}

vector<NodeIter> AlignedTree::GetSplitDescendants(const NodeIter& node) const {
  vector<NodeIter> descendants;
  for (NodeIter it = node.begin(); it != node.end(); ++it) {
    if (it->IsSplitNode()) {
      descendants.push_back(it);
      it.skip_children();
    }
  }

  return descendants;
}

void AlignedTree::DisplayTree(Dictionary& dictionary) const {
  int current_depth = 0;
  for (auto it = begin_breadth_first(); it != end_breadth_first(); ++it) {
    if (depth(it) != current_depth) {
      cerr << endl;
      current_depth = depth(it);
    }
    cerr << dictionary.GetToken(it->GetTag()) << " ";
    if (it->IsSetWord() && (!it->IsSplitNode() || it == begin())) {
      cerr << dictionary.GetToken(it->GetWord()) << " ";
    }
  }
  cerr << endl;
}

void AlignedTree::Write(ostream& out, Dictionary& dictionary) const {
  int var_index = 0;
  Write(out, begin(), dictionary, var_index);
}

void AlignedTree::Write(ostream& out, const iterator& root,
                        Dictionary& dictionary, int& var_index) const {
  out << "(" << dictionary.GetToken(root->GetTag()) << " ";

  if (root.number_of_children() == 0) {
    if (root->IsSetWord()) {
      out << dictionary.GetToken(root->GetWord());
    } else {
      out << "#" << var_index++;
    }
  } else {
    for (auto child = begin(root); child != end(root); ++child) {
      Write(out, child, dictionary, var_index);
      out << " ";
    }
  }

  out << ")";
}

bool AlignedTree::operator<(const AlignedTree& tree) const {
  if (size() != tree.size()) {
    return size() < tree.size();
  }

  for (auto it1 = begin(), it2 = tree.begin();
       it1 != end() && it2 != tree.end();
       ++it1, ++it2) {
    if (it1.number_of_children() != it2.number_of_children()) {
      return it1.number_of_children() < it2.number_of_children();
    }

    if (*it1 != *it2) {
      return *it1 < *it2;
    }
  }

  return false;
}

bool AlignedTree::operator==(const AlignedTree& tree) const {
  return !(*this < tree || tree < *this);
}

bool operator<(const NodeIter& it1, const NodeIter& it2) {
  // Comparing pointers.
  return it1.node < it2.node;
}
