#include "translation_table.h"

#include <cmath>

#include "dictionary.h"

const double TranslationTable::DEFAULT_NULL_PROB = 1e-2;

TranslationTable::TranslationTable(
    ifstream& fin, Dictionary& dict, bool rev, int max_threads) :
    cache(max_threads) {
  double prob;
  string src, trg;
  while (fin >> src >> trg >> prob) {
    prob = exp(prob);
    if (rev) swap(src,trg);
    if (src == "<eps>") src == "__NULL__";
    if (trg == "<eps>") trg == "__NULL__";
    int source_word = dict.GetIndex(src);
    int target_word = dict.GetIndex(trg);
    if (prob >= DEFAULT_NULL_PROB) {
      table[make_pair(source_word, target_word)] = prob;
    }
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

double TranslationTable::GetProbability(
    int source_word, int target_word) const {
  if (source_word == Dictionary::NULL_WORD_ID) {
    return DEFAULT_NULL_PROB;
  }

  auto result = table.find(make_pair(source_word, target_word));
  if (result != table.end()) {
    return result->second;
  }

  return 0;
}
