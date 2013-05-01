#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class Dictionary {
 public:
  int GetIndex(const string& token);

  string GetToken(int index);

 private:
  unordered_map<string, int> tokens_index;
  vector<string> tokens;
};

#endif
