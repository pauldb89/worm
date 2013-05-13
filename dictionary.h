#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class Dictionary {
 public:
  Dictionary();

  Dictionary(ifstream& fin);

  int GetIndex(const string& token);

  string GetToken(int index);

  static const string NULL_WORD;
  static const int NULL_WORD_ID;

 private:
  void Initialize();

  unordered_map<string, int> tokens_index;
  vector<string> tokens;
};

#endif
