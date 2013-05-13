#include "translation_table.h"

#include "dictionary.h"

const double TranslationTable::DEFAULT_NULL_PROB = 1e-50;

TranslationTable::TranslationTable(
    ifstream& fin, Dictionary& source_vocabulary,
    Dictionary& target_vocabulary, Dictionary& dict) {
  double prob;
  int source_id, target_id;
  while (fin >> target_id >> source_id >> prob) {
    int source_word = dict.GetIndex(source_vocabulary.GetToken(source_id));
    int target_word = dict.GetIndex(target_vocabulary.GetToken(target_id));
    table[make_pair(source_word, target_word)] = prob;
  }
}

void TranslationTable::CacheSentence(const vector<int>& source_words,
                                     const vector<int>& target_words) {
  cache = vector<vector<double>>(source_words.size());
  for (size_t i = 0; i < source_words.size(); ++i) {
    cache[i].resize(target_words.size() + 1);

    for (size_t j = 0; j < target_words.size(); ++j) {
      auto it = table.find(make_pair(source_words[i], target_words[j]));
      cache[i][j] = it == table.end() ? 0 : it->second;
    }

    auto it = table.find(make_pair(source_words[i], Dictionary::NULL_WORD_ID));
    cache[i][target_words.size()] = it == table.end() ?
        DEFAULT_NULL_PROB : it->second;
  }
}

double TranslationTable::ComputeLogProbability(
    const vector<int>& source_indexes, const vector<int>& target_indexes) {
  double result = 0;
  for (auto source_index: source_indexes) {
    double sum = cache[source_index].back();
    for (auto target_index: target_indexes) {
      sum += cache[source_index][target_index];
    }
    result += log(sum / (target_indexes.size() + 1));
  }

  return result;
}
