#include "translation_table.h"

#include "dictionary.h"

const double TranslationTable::DEFAULT_NULL_PROB = 1e-3;

TranslationTable::TranslationTable(
    ifstream& fin, Dictionary& source_vocabulary,
    Dictionary& target_vocabulary, Dictionary& dict, int max_threads) :
    cache(max_threads) {
  double prob;
  int source_id, target_id;
  while (fin >> source_id >> target_id >> prob) {
    int source_word = dict.GetIndex(source_vocabulary.GetToken(source_id));
    int target_word = dict.GetIndex(target_vocabulary.GetToken(target_id));
    table[make_pair(source_word, target_word)] = prob;
  }
}

void TranslationTable::CacheSentence(
    const vector<int>& source_words,
    const vector<int>& target_words,
    int thread_id) {
  cache[thread_id] = vector<vector<double>>(target_words.size());
  for (size_t i = 0; i < target_words.size(); ++i) {
    cache[thread_id][i].resize(source_words.size() + 1);
    for (size_t j = 0; j < source_words.size(); ++j) {
      cache[thread_id][i][j] = GetProbability(source_words[j], target_words[i]);
    }
    cache[thread_id][i].back() =
        GetProbability(Dictionary::NULL_WORD_ID, target_words[i]);
  }
}

double TranslationTable::ComputeAverageLogProbability(
    const vector<int>& source_indexes,
    const vector<int>& target_indexes,
    int thread_id) {
  double result = 0;
  for (auto target_index: target_indexes) {
    double best = cache[thread_id][target_index].back();
    for (auto source_index: source_indexes) {
      best = max(best, cache[thread_id][target_index][source_index]);
    }
    result += log(best);
  }

  return result;
}

double TranslationTable::GetProbability(int source_word, int target_word) {
  auto result = table.find(make_pair(source_word, target_word));
  if (result != table.end()) {
    return result->second;
  }

  if (source_word == Dictionary::NULL_WORD_ID) {
    return DEFAULT_NULL_PROB;
  }

  return 0;
}
