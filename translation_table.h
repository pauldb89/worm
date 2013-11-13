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
  TranslationTable(ifstream& fin, Dictionary& source_vocabulary,
                   Dictionary& target_vocabulary, Dictionary& dictionary,
                   int max_threads);

  void CacheSentence(
      const vector<int>& source_words,
      const vector<int>& target_words,
      int thread_id);

  double ComputeAverageLogProbability(
      const vector<int>& source_indexes,
      const vector<int>& target_indexes,
      int thread_id);

  double GetProbability(int source_word, int target_word);

  static const double DEFAULT_NULL_PROB;

 private:
  unordered_map<pair<int, int>, double, PairHash> table;
  vector<vector<vector<double>>> cache;
};

#endif
