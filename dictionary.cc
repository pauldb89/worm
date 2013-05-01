#include "dictionary.h"

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
