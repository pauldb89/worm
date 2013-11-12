#ifndef _SAMPLER_H_
#define _SAMPLER_H_

#include <memory>
#include <random>
#include <unordered_map>

#include "aligned_tree.h"
#include "dictionary.h"
#include "synchronized_restaurant.h"
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
          const shared_ptr<TranslationTable>& reverse_table,
          RandomGenerator& generator, bool enable_all_stats,
          int min_rule_count, double alpha,
          double pexpand, double pchild, double pterm);

  void Sample(const string& output_prefix, int iterations, int log_frequency);

  void SerializeAlignments(const string& output_prefix);

  void SerializeGrammar(const string& output_prefix, bool scfg_format);

 private:
  void InitializeRuleCounts();

  void CacheSentence(const Instance& instance);

  void DisplayStats();

  double ComputeDataLikelihood();

  double ComputeAverageNumInteriorNodes();

  int GetGrammarSize();

  map<int, int> GenerateRuleHistogram();

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

  Alignment ConstructNonterminalLinks(const Rule& rule);

  pair<Alignment, Alignment> ConstructTerminalLinks(const Rule& rule);

  pair<Alignment, Alignment> ConstructAlignments(const Rule& rule);

  shared_ptr<vector<Instance>> training;
  vector<shared_ptr<Instance>> initial_order;
  SynchronizedRuleCounts counts;

  Dictionary& dictionary;
  shared_ptr<PCFGTable> pcfg_table;
  shared_ptr<TranslationTable> forward_table;
  shared_ptr<TranslationTable> reverse_table;
  RandomGenerator& generator;
  uniform_real_distribution<double> uniform_distribution;

  bool enable_all_stats;

  // Parameters for filtering the final rules.
  int min_rule_count;

  double alpha;
  double prob_expand, prob_not_expand;
  double prob_stop_child, prob_cont_child;
  double prob_stop_str, prob_cont_str;
  double prob_nt, prob_st, prob_tt;
};

#endif
