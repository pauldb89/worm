#include "lattice.h"

#include "dictionary.h"

Lattice::Lattice(const Distribution& distribution, Dictionary& dictionary) {
  int num_words = distribution[0].first.size();
  int sink_node = (num_words - 1) * distribution.size() + 1;
  graph.resize(sink_node + 1);
  for (size_t i = 0; i < distribution.size(); ++i) {
    string word = dictionary.GetToken(distribution[i].first.front().GetWord());
    graph[0].push_back(make_tuple(
        word, distribution[i].second, i * (num_words - 1) + 1));
    for (int j = 1; j < num_words - 1; ++j) {
      string word = dictionary.GetToken(distribution[i].first[j].GetWord());
      graph[i * (num_words - 1) + j].push_back(make_tuple(
          word, 1, i * (num_words - 1) + j + 1));
    }
    word = dictionary.GetToken(distribution[i].first.back().GetWord());
    graph[(i + 1) * (num_words - 1)].push_back(make_tuple(word, 1, sink_node));
  }
}
