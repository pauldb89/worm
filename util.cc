#include "util.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>

#include <boost/regex.hpp>

#include "aligned_tree.h"
#include "dictionary.h"

Instance ReadInstance(ifstream& tree_infile,
                      ifstream& string_infile,
                      Dictionary& dictionary) {
  AlignedTree tree = ReadParseTree(tree_infile, dictionary);
  String sentence = ReadString(string_infile, dictionary);
  tree.begin()->SetSplitNode(true);
  tree.begin()->SetSpan(make_pair(0, sentence.size()));
  return Instance(tree, sentence);
}

AlignedTree ReadParseTree(ifstream& tree_infile, Dictionary& dictionary) {
  AlignedTree tree;
  string line;
  getline(tree_infile, line);

  // TODO(pauldb): Replace with std::regex and std::sregex_token_iterator when
  // g++ will support both.
  boost::regex r("[()]|[^\\s()][^\\s]*[^\\s()]|[^\\s()]+");
  boost::sregex_token_iterator begin(line.begin(), line.end(), r), end;
  stack<AlignedTree::iterator> st;
  for (auto it = begin; it != end; ++it) {
    // We need to careful with "(" and ")" symbols in the original sentence.
    // (That's why we have complicated checks for entering and leaving a
    // subtree.)
    if (*it == "(" && *next(it) != ")") {
      // Create an empty node and add it to the stack (enter subtree).
      if (st.empty()) {
        // Create root node.
        st.push(tree.insert(tree.begin(), AlignedNode()));
      } else {
        // Insert a new child to the node at top of the stack.
        st.push(tree.append_child(st.top(), AlignedNode()));
      }
    } else if (*it == ")" &&
               (st.top()->IsSetWord() || st.top().number_of_children())) {
      // Remove node from the top of the stack (leave subtree).
      st.pop();
    } else if (!st.top()->IsSetTag()) {
      // If the top node is empty (i.e. the tag is unset), we are reading the
      // nonterminal root of the subtree.
      st.top()->SetTag(dictionary.GetIndex(*it));
    } else {
      // Otherwise, we are reading a leaf node (terminal).
      st.top()->SetWord(dictionary.GetIndex(*it));
    }
  }

  assert(st.empty());

  return tree;
}

String ReadString(ifstream& string_infile, Dictionary& dictionary) {
  String sentence;
  string line;
  getline(string_infile, line);
  istringstream iss(line);
  string word;
  while (iss >> word) {
    sentence.push_back(StringNode(dictionary.GetIndex(word), -1));
  }

  return sentence;
}
