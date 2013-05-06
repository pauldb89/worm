#include "aligned_tree.h"

typedef AlignedTree::iterator NodeIter;

vector<AlignedNode> AlignedTree::GetVariables() {
  vector<AlignedNode> variables;
  for (auto node: *this) {
    if (node.IsSplitNode()) {
      variables.push_back(node);
    }
  }

  return variables;
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
  for (auto sibling = begin(node); sibling != end(node); ++sibling) {
    if (!sibling->IsSplitNode()) {
      auto new_node = fragment.append_child(fragment_node);
      ConstructFragment(sibling, fragment, new_node);
    }
  }
}

vector<NodeIter> AlignedTree::GetSplitDescendants(const NodeIter& node) const {
  vector<NodeIter> descendants;
  for (auto it = node.begin(); it != node.end(); ++it) {
    if (it->IsSplitNode()) {
      descendants.push_back(it);
      it.skip_children();
    }
  }
  return descendants;
}
