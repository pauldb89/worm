#ifndef _SAMPLER_H_
#define _SAMPLER_H_

#include <memory>
#include <random>
#include <unordered_map>

#include "aligned_tree.h"
#include "dictionary.h"
#include "restaurant.h"
#include "util.h"

using namespace std;

typedef Restaurant<Rule> RuleCounts;
typedef AlignedTree::iterator NodeIter;

class PCFGTable;
class TranslationTable;

class Sampler {
 public:
  Sampler(const shared_ptr<vector<Instance>>& training, Dictionary& dictionary,
          const shared_ptr<PCFGTable>& pcfg_table,
          const shared_ptr<TranslationTable>& forward_table,
          const shared_ptr<TranslationTable>& backward_table,
          RandomGenerator& generator, double alpha, double pexpand,
          double pchild, double pterm);

  void Sample(int iterations);

  void SerializeGrammar(ofstream& out);

 private:
  void InitializeRuleCounts();

  void CacheSentence(const Instance& instance);

  double ComputeDataLikelihood();

  void SampleAlignments(const Instance& instance);

  void SampleSwaps(const Instance& instance);

  vector<NodeIter> GetRandomSchedule(const AlignedTree& tree);

  Rule GetRule(const Instance& instance, const NodeIter& node);

  String ConstructRuleTargetSide(const AlignedTree& fragment,
                                 const String& target_string);

  vector<pair<int, int>> GetLegalSpans(const AlignedTree& tree,
                                       const NodeIter& node,
                                       const NodeIter& ancestor);

  double ComputeLogBaseProbability(const Rule& rule);

  double ComputeLogProbability(const Rule& r);

  double ComputeLogProbability(const Rule& r1, const Rule& r2);

  double ComputeLogProbability(const Rule& r1, const Rule& r2, const Rule& r3);

  void IncrementRuleCount(const Rule& rule);

  void DecrementRuleCount(const Rule& rule);

  shared_ptr<vector<Instance>> training;
  unordered_map<int, RuleCounts> counts;

  Dictionary& dictionary;
  shared_ptr<PCFGTable> pcfg_table;
  shared_ptr<TranslationTable> forward_table;
  shared_ptr<TranslationTable> backward_table;
  RandomGenerator& generator;
  uniform_real_distribution<double> uniform_distribution;

  double alpha;
  double prob_expand, prob_not_expand;
  double prob_stop_child, prob_cont_child;
  double prob_stop_str, prob_cont_str;
  double prob_nt, prob_st, prob_tt;
};

#endif
