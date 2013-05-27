#include "util.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>

#include <boost/regex.hpp>

#include "aligned_tree.h"
#include "dictionary.h"

typedef AlignedTree::iterator NodeIter;

Instance ReadInstance(ifstream& tree_stream,
                      ifstream& string_stream,
                      ifstream& alignment_stream,
                      Dictionary& dictionary) {
  AlignedTree tree = ReadParseTree(tree_stream, dictionary);
  String target_string = ReadTargetString(string_stream, dictionary);
  ConstructGHKMDerivation(tree, target_string, alignment_stream, dictionary);
  return Instance(tree, target_string);
}

AlignedTree ReadParseTree(ifstream& tree_stream, Dictionary& dictionary) {
  AlignedTree tree;
  string line;
  getline(tree_stream, line);

  int word_index = 0;
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
      st.top()->SetWordIndex(word_index);
      ++word_index;
    }
  }

  assert(st.empty());

  return tree;
}

String ReadTargetString(ifstream& string_stream, Dictionary& dictionary) {
  String target_string;
  string line;
  getline(string_stream, line);
  istringstream iss(line);
  string word;

  int word_index = 0;
  while (iss >> word) {
    target_string.push_back(StringNode(dictionary.GetIndex(word),
                                       word_index, -1));
    ++word_index;
  }

  return target_string;
}

void ConstructGHKMDerivation(AlignedTree& tree,
                             const String& target_string,
                             ifstream& alignment_stream,
                             Dictionary& dictionary) {
  string line;
  getline(alignment_stream, line);

  // Ignore alignments for sentences that are impossible to parse.
  if (tree.size() == 1) {
    return;
  }

  boost::regex r("[0-9]+");
  boost::sregex_token_iterator it(line.begin(), line.end(), r), end;
  int source_size = tree.size(), target_size = target_string.size();
  vector<pair<int, int>> forward(source_size, make_pair(target_size, 0));
  vector<pair<int, int>> backward(target_size, make_pair(source_size, 0));
  while (it != end) {
    int x = stoi(*(it++));
    int y = stoi(*(it++));
    forward[x].first = min(forward[x].first, y);
    forward[x].second = max(forward[x].second, y + 1);
    backward[y].first = min(backward[y].first, x);
    backward[y].second = max(backward[y].second, x + 1);
  }

  map<NodeIter, pair<int, int>> source_spans, target_spans;
  for (auto node = tree.begin_post(); node != tree.end_post(); ++node) {
    auto source_span = make_pair(source_size, 0);
    auto target_span = make_pair(target_size, 0);
    if (node->IsSetWord()) {
      int word_index = node->GetWordIndex();
      source_span = make_pair(word_index, word_index + 1);
      target_span = forward[word_index];
    } else {
      for (auto child = tree.begin(node); child != tree.end(node); ++child) {
        source_span = make_pair(
            min(source_span.first, source_spans[child].first),
            max(source_span.second, source_spans[child].second));
        target_span = make_pair(
            min(target_span.first, target_spans[child].first),
            max(target_span.second, target_spans[child].second));
      }
    }
    source_spans[node] = source_span;
    target_spans[node] = target_span;

    if (target_span.first >= target_span.second) {
      node->SetSplitNode(false);
      node->SetSpan(make_pair(-1, -1));
      continue;
    }

    node->SetSplitNode(true);
    for (int i = target_span.first; i < target_span.second; ++i) {
      if (backward[i].first < source_span.first ||
          backward[i].second > source_span.second) {
        node->SetSplitNode(false);
        break;
      }
    }
    if (node->IsSplitNode()) {
      node->SetSpan(target_span);
    }
  }

  tree.begin()->SetSplitNode(true);
  tree.begin()->SetSpan(make_pair(0, target_size));
}

void WriteSCFGRule(ofstream& out, const Rule& rule, Dictionary& dictionary) {
  const AlignedTree& tree = rule.first;
  out << dictionary.GetToken(tree.GetRootTag()) << " ||| ";

  for (auto leaf = tree.begin_leaf(); leaf != tree.end_leaf(); ++leaf) {
    if (leaf->IsSetWord() && (leaf == tree.begin() || !leaf->IsSplitNode())) {
      out << dictionary.GetToken(leaf->GetWord()) << " ";
    } else {
      out << dictionary.GetToken(leaf->GetTag()) << " ";
    }
  }
  out << "||| ";

  for (auto node: rule.second) {
    if (node.IsSetWord()) {
      out << dictionary.GetToken(node.GetWord()) << " ";
    } else {
      out << "#" << node.GetVarIndex() << " ";
    }
  }
  out << "||| ";
}

ofstream& operator<<(ofstream& out, const Alignment& alignment) {
  for (auto link: alignment) {
    out << link.first << "-" << link.second << " ";
  }
  return out;
}
