#ifndef _TRANSLATION_TABLE_H_
#define _TRANSLATION_TABLE_H_

#include <fstream>
#include <unordered_map>
#include <vector>

#include <boost/functional/hash.hpp>

using namespace std;

class Dictionary;

typedef boost::hash<pair<int, int>> PairHash;

class TranslationTable {
 public:
  TranslationTable(ifstream& fin, Dictionary& dictionary);

  double ComputeLogProbability(const vector<int>& source_words,
                               const vector<int>& target_words);

 private:
  unordered_map<pair<int, int>, double, PairHash> table;
  int null_word;
};

#endif
