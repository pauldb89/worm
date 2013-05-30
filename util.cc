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

Instance ReadInstance(istream& tree_stream,
                      istream& string_stream,
                      istream& alignment_stream,
                      Dictionary& dictionary) {
  AlignedTree tree = ReadParseTree(tree_stream, dictionary);
  String target_string = ReadTargetString(string_stream, dictionary);
  ConstructGHKMDerivation(tree, target_string, alignment_stream, dictionary);
  return Instance(tree, target_string);
}

AlignedTree ReadParseTree(istream& tree_stream, Dictionary& dictionary) {
  AlignedTree tree;
  string line;
  getline(tree_stream, line);

  int word_index = 0;
  // TODO(pauldb): Replace with std::regex and std::sregex_token_iterator when
  // g++ will support both.
  boost::regex var_index("#[0-9]+");
  boost::regex token("[()]|[^\\s()][^\\s]*[^\\s()]|[^\\s()]+");
  boost::sregex_token_iterator begin(line.begin(), line.end(), token), end;
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
               (st.top()->IsSetWord() || st.top()->IsSplitNode() ||
                st.top().number_of_children())) {
      // Remove node from the top of the stack (leave subtree).
      st.pop();
    } else if (!st.top()->IsSetTag()) {
      // If the top node is empty (i.e. the tag is unset), we are reading the
      // nonterminal root of the subtree.
      st.top()->SetTag(dictionary.GetIndex(*it));
    } else {
      // Otherwise, we are reading a leaf node (terminal or variable index).
      if (boost::regex_match((string) *it, var_index)) {
        st.top()->SetSplitNode(true);
      } else {
        st.top()->SetWord(dictionary.GetIndex(*it));
        st.top()->SetWordIndex(word_index);
        ++word_index;
      }
    }
  }

  assert(st.empty());

  return tree;
}

String ReadTargetString(istream& string_stream, Dictionary& dictionary) {
  String target_string;
  string line;
  getline(string_stream, line);

  istringstream iss(line);
  boost::regex var_index("#[0-9]+");
  string word;
  int word_index = 0;
  while (iss >> word) {
    if (boost::regex_match(word, var_index)) {
      target_string.push_back(StringNode(-1, -1, stoi(word.substr(1))));
    } else {
      int word_id = dictionary.GetIndex(word);
      target_string.push_back(StringNode(word_id, word_index, -1));
      ++word_index;
    }
  }

  return target_string;
}

pair<Rule, double> ReadRule(istream& grammar_stream, Dictionary& dictionary) {
  string line;
  getline(grammar_stream, line);

  boost::regex separator("\\|\\|\\|");
  boost::sregex_token_iterator it(line.begin(), line.end(), separator, -1);

  // Ignore root tag.
  istringstream tree_stream(*(++it));
  AlignedTree tree = ReadParseTree(tree_stream, dictionary);

  istringstream string_stream(*(++it));
  String target_string = ReadTargetString(string_stream, dictionary);

  istringstream score_stream(*(++it));
  double score;
  score_stream >> score;
  return make_pair(make_pair(tree, target_string), score);
}

istream& operator>>(istream& in, Alignment& alignment) {
  string line;
  getline(in, line);

  boost::regex number("[0-9]+");
  boost::sregex_token_iterator it(line.begin(), line.end(), number), end;
  while (it != end) {
    int x = stoi(*(it++));
    int y = stoi(*(it++));
    alignment.push_back(make_pair(x, y));
  }

  return in;
}

void ConstructGHKMDerivation(AlignedTree& tree,
                             const String& target_string,
                             istream& alignment_stream,
                             Dictionary& dictionary) {
  Alignment alignment;
  alignment_stream >> alignment;

  // Ignore alignments for sentences that are impossible to parse.
  if (tree.size() == 1) {
    return;
  }

  int source_size = tree.size(), target_size = target_string.size();
  vector<pair<int, int>> forward_links(source_size, make_pair(target_size, 0));
  vector<pair<int, int>> backward_links(target_size, make_pair(source_size, 0));
  for (auto link: alignment) {
    int x = link.first, y = link.second;
    forward_links[x].first = min(forward_links[x].first, y);
    forward_links[x].second = max(forward_links[x].second, y + 1);
    backward_links[y].first = min(backward_links[y].first, x);
    backward_links[y].second = max(backward_links[y].second, x + 1);
  }

  map<NodeIter, pair<int, int>> source_spans, target_spans;
  for (auto node = tree.begin_post(); node != tree.end_post(); ++node) {
    auto source_span = make_pair(source_size, 0);
    auto target_span = make_pair(target_size, 0);
    if (node->IsSetWord()) {
      int word_index = node->GetWordIndex();
      source_span = make_pair(word_index, word_index + 1);
      target_span = forward_links[word_index];
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
      if (backward_links[i].first < source_span.first ||
          backward_links[i].second > source_span.second) {
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

void WriteTargetString(ostream& out,
                       const String& target_string,
                       Dictionary& dictionary) {
  for (auto node: target_string) {
    if (node.IsSetWord()) {
      out << dictionary.GetToken(node.GetWord()) << " ";
    } else {
      out << "#" << node.GetVarIndex() << " ";
    }
  }
}

void WriteSCFGRule(ostream& out, const Rule& rule, Dictionary& dictionary) {
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

  WriteTargetString(out, rule.second, dictionary);
}

void WriteSTSGRule(ostream& out, const Rule& rule, Dictionary& dictionary) {
  const AlignedTree& tree = rule.first;
  out << dictionary.GetToken(tree.GetRootTag()) << " ||| ";
  tree.Write(out, dictionary);
  out << " ||| ";
  WriteTargetString(out, rule.second, dictionary);
}

ostream& operator<<(ostream& out, const Alignment& alignment) {
  for (auto link: alignment) {
    out << link.first << "-" << link.second << " ";
  }
  return out;
}
