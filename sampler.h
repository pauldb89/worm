#ifndef _SAMPLER_H_
#define _SAMPLER_H_

#include <memory>
#include <random>
#include <unordered_map>

#include "aligned_tree.h"
#include "dictionary.h"
#include "distributed_rule_counts.h"
#include "rule_reorderer.h"
#include "util.h"

using namespace std;

class PCFGTable;
class TranslationTable;

typedef mt19937 RandomGenerator;

class Sampler {
 public:
  Sampler(const shared_ptr<vector<Instance>>& training, Dictionary& dictionary,
          const shared_ptr<PCFGTable>& pcfg_table,
          const shared_ptr<TranslationTable>& forward_table,
          const shared_ptr<TranslationTable>& reverse_table,
          RandomGenerator& generator, int num_threads, bool enable_all_stats,
          bool smart_expand, int min_rule_count, bool reorder, double penalty,
          int max_leaves, int max_tree_size, double alpha,
          double pexpand, double pchild, double pterm,
          const string& output_directory);

  void Sample(int iterations, int log_frequency);

  void SerializeAlignments(const string& iteration = "");

  void SerializeGrammar(bool scfg_format, const string& iteration = "");

  void SerializeReorderings(const string& iteration = "");

 private:
  void InitializeRuleCounts();

  void CacheSentence(const Instance& instance);

  void DisplayStats();

  double ComputeDataLikelihood();

  double ComputeAverageNumInteriorNodes();

  int GetGrammarSize();

  map<int, int> GenerateRuleHistogram();

  void SampleAlignments(const Instance& instance, int index);

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

  void InferReorderings();

  void ExtractReordering(const Instance& instance,
                         const NodeIter& node,
                         String& reordering);

  string GetOutputFilename(
      const string& iteration, const string& extension) const;

  shared_ptr<vector<Instance>> training;
  DistributedRuleCounts counts;

  Dictionary& dictionary;
  shared_ptr<PCFGTable> pcfg_table;
  shared_ptr<TranslationTable> forward_table;
  shared_ptr<TranslationTable> reverse_table;
  RandomGenerator& generator;
  uniform_real_distribution<double> uniform_distribution;

  int num_threads;
  bool enable_all_stats;
  // Parameters for filtering the final rules.
  int min_rule_count;

  bool reorder;
  RuleReorderer rule_reorderer;
  vector<map<String, int>> reorder_counts;

  bool smart_expand;
  unordered_map<int, double> expand_probs;
  unordered_map<int, double> not_expand_probs;
  double alpha;
  double prob_expand, prob_not_expand;
  double prob_stop_child, prob_cont_child;
  double prob_stop_str, prob_cont_str;
  double prob_nt, prob_st, prob_tt;

  string output_directory;
};

#endif
