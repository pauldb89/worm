#include "node.h"

AlignedNode::AlignedNode() : tag(-1), word(-1), start(0), end(0),
                             split_node(false) {}

bool AlignedNode::IsSetTag() {
  return tag == -1;
}

int AlignedNode::GetTag() {
  return tag;
}

void AlignedNode::SetTag(int value) {
  tag = value;
}

int AlignedNode::GetWord() {
  return word;
}

void AlignedNode::SetWord(int value) {
  word = value;
}

StringNode::StringNode(int word, int var_index) :
                       word(word), var_index(var_index) {}

int StringNode::GetWord() {
  return word;
}
