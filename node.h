#ifndef _NODE_H_
#define _NODE_H_

#include <utility>

using namespace std;

class AlignedNode {
 public:
  AlignedNode();

  bool IsSetTag() const;

  int GetTag() const;

  void SetTag(int value);

  bool IsSetWord() const;

  int GetWord() const;

  void SetWord(int value);

  void UnsetWord();

  int GetWordIndex() const;

  void SetWordIndex(int value);

  bool IsSplitNode() const;

  void SetSplitNode(bool value);

  pair<int, int> GetSpan() const;

  void SetSpan(const pair<int, int>& span);

  bool operator<(const AlignedNode& node) const;

  bool operator!=(const AlignedNode& node) const;

 private:
  int tag, word, word_index;
  int start, end;
  bool split_node;
};

class StringNode {
 public:
  StringNode(int word, int word_index, int var_index);

  bool IsSetWord() const;

  int GetWord() const;

  int GetWordIndex() const;

  void SetWordIndex(int value);

  int GetVarIndex() const;

  bool operator<(const StringNode& node) const;

  bool operator==(const StringNode& node) const;

 private:
  int word, word_index, var_index;
};

#endif
