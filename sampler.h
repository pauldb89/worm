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

typedef pair<AlignedTree, String> Rule;
typedef Restaurant<Rule> RuleCounts;
typedef AlignedTree::iterator NodeIter;

class Sampler {
 public:
  Sampler(const shared_ptr<vector<Instance>>& training, double alpha,
          double pexpand, double pchild, double pterm,
          RandomGenerator& generator, Dictionary& dictionary);

  void Sample(int iterations);

 private:
  void InitializeRuleCounts();

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

  double prob_expand, prob_not_expand;
  double prob_stop_child, prob_cont_child;
  double prob_stop_str, prob_cont_str;
  double prob_nt, prob_st, prob_tt;

  RandomGenerator& generator;
  uniform_real_distribution<double> uniform_distribution;

  // TODO(pauldb): Remove dictionary if it's only used for debugging.
  Dictionary& dictionary;
};

#endif
