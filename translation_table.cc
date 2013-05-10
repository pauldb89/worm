#include "translation_table.h"

#include "dictionary.h"

TranslationTable::TranslationTable(ifstream& fin, Dictionary& dictionary) {
  null_word = dictionary.GetIndex("NULL");

  double prob;
  string source_word, target_word;
  while (fin >> source_word >> target_word >> prob) {
    int source_word_index = dictionary.GetIndex(source_word);
    int target_word_index = dictionary.GetIndex(target_word);
    table[make_pair(source_word_index, target_word_index)] = prob;
  }
}

double TranslationTable::ComputeLogProbability(
    const vector<int>& source_words, const vector<int>& target_words) {
  double result = 0;
  for (auto source_word: source_words) {
    double sum = 0;
    for (auto target_word: target_words) {
      auto it = table.find(make_pair(source_word, target_word));
      if (it != table.end()) {
        sum += it->second;
      }
    }

    auto it = table.find(make_pair(source_word, null_word));
    if (it != table.end()) {
      sum += it->second;
    }

    result += log(sum / target_words.size());
  }

  return result;
}
