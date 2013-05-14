#include "node.h"

AlignedNode::AlignedNode() :
    tag(-1), word(-1), word_index(-1), start(-1), end(-1), split_node(false) {}

bool AlignedNode::IsSetTag() {
  return tag != -1;
}

int AlignedNode::GetTag() {
  return tag;
}

void AlignedNode::SetTag(int value) {
  tag = value;
}

bool AlignedNode::IsSetWord() {
  return word != -1;
}

int AlignedNode::GetWord() {
  return word;
}

void AlignedNode::SetWord(int value) {
  word = value;
}

void AlignedNode::UnsetWord() {
  word = word_index = -1;
}

int AlignedNode::GetWordIndex() {
  return word_index;
}

void AlignedNode::SetWordIndex(int value) {
  word_index = value;
}

bool AlignedNode::IsSplitNode() {
  return split_node;
}

void AlignedNode::SetSplitNode(bool value) {
  split_node = value;
}

pair<int, int> AlignedNode::GetSpan() {
  return make_pair(start, end);
}

void AlignedNode::SetSpan(const pair<int, int>& span) {
  start = span.first;
  end = span.second;
}

bool AlignedNode::operator<(const AlignedNode& node) const {
  return tag < node.tag || (tag == node.tag && word < node.word);
}

bool AlignedNode::operator!=(const AlignedNode& node) const {
  return tag != node.tag || word != node.word;
}


StringNode::StringNode(int word, int word_index, int var_index) :
    word(word), word_index(word_index), var_index(var_index) {}

bool StringNode::IsSetWord() const {
  return word != -1;
}

int StringNode::GetWord() const {
  return word;
}

int StringNode::GetWordIndex() const {
  return word_index;
}

int StringNode::GetVarIndex() const {
  return var_index;
}

bool StringNode::operator<(const StringNode& node) const {
  return word < node.word || (word == node.word && var_index < node.var_index);
}

bool StringNode::operator==(const StringNode& node) const {
  return word == node.word && var_index == node.var_index;
}
