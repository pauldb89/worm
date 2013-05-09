#ifndef _NODE_H_
#define _NODE_H_

#include <utility>

using namespace std;

class AlignedNode {
 public:
  AlignedNode();

  bool IsSetTag();

  int GetTag();

  void SetTag(int value);

  bool IsSetWord();

  int GetWord();

  void SetWord(int value);

  bool IsSplitNode();

  void SetSplitNode(bool value);

  pair<int, int> GetSpan();

  void SetSpan(const pair<int, int>& span);

  bool operator<(const AlignedNode& node) const;

  bool operator!=(const AlignedNode& node) const;

 private:
  int tag, word;
  int start, end;
  bool split_node;
};

class StringNode {
 public:
  StringNode(int word, int var_index);

  bool IsSetWord() const;

  int GetWord() const;

  int GetVarIndex() const;

  bool operator<(const StringNode& node) const;

  bool operator==(const StringNode& node) const;

 private:
  int word, var_index;
};

#endif
