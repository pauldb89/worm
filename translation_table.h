#ifndef _TRANSLATION_TABLE_H_
#define _TRANSLATION_TABLE_H_

#include <fstream>
#include <unordered_map>
#include <vector>

#include <boost/functional/hash.hpp>

using namespace std;

class Dictionary;

typedef boost::hash<pair<int, int>> PairHasher;

class TranslationTable {
 public:
  TranslationTable(ifstream& fin, Dictionary& dictionary, bool reversed,
                   int max_threads);

  void CacheSentence(
      const vector<int>& source_words,
      const vector<int>& target_words,
      int thread_id);

  double ComputeAverageLogProbability(
      const vector<int>& source_indexes,
      const vector<int>& target_indexes,
      int thread_id);

  double GetProbability(int source_word, int target_word) const;

  static const double DEFAULT_NULL_PROB;

 private:
  unordered_map<pair<int, int>, double, PairHasher> table;
  vector<vector<vector<double>>> cache;
};

#endif
