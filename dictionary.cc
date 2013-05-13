#include "dictionary.h"

#include <cassert>

const string Dictionary::NULL_WORD = "__NULL__";
const int Dictionary::NULL_WORD_ID = 0;

Dictionary::Dictionary() {
  Initialize();
}

Dictionary::Dictionary(ifstream& fin) {
  Initialize();

  string word;
  int word_id, word_counts;
  while (fin >> word_id >> word >> word_counts) {
    assert(word_id == (int) tokens.size());
    tokens.push_back(word);
    tokens_index[word] = word_id;
  }
}

void Dictionary::Initialize() {
  tokens.push_back(NULL_WORD);
  tokens_index[NULL_WORD] = NULL_WORD_ID;
}

int Dictionary::GetIndex(const string& token) {
  if (tokens_index.count(token)) {
    return tokens_index[token];
  }

  int index = tokens.size();
  tokens_index[token] = index;
  tokens.push_back(token);
  return index;
}

string Dictionary::GetToken(int index) {
  return tokens[index];
}
