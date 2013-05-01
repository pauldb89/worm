#ifndef _NODE_H_
#define _NODE_H_

class AlignedNode {
 public:
  AlignedNode();

  bool IsSetTag();

  int GetTag();

  void SetTag(int tag);

  int GetWord();

  void SetWord(int word);

 private:
  int tag, word;
  int start, end;
  bool split_node;
};

class StringNode {
 public:
  StringNode(int word, int var_index);

  int GetWord();

 private:
  int word, var_index;
};

#endif
