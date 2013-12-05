#include "distributed_rule_counts.h"

#include <chrono>

#include <omp.h>

#include "aligned_tree.h"

using namespace chrono;

typedef high_resolution_clock Clock;

DistributedRuleCounts::DistributedRuleCounts(int max_threads, double alpha) :
    rule_counts(max_threads), snapshot(max_threads), alpha(alpha) {}

void DistributedRuleCounts::AddNonterminal(int nonterminal) {
  AddNonterminal(rule_counts, nonterminal);
  if (!snapshot.count(nonterminal)) {
    snapshot[nonterminal] = RestaurantProcess<Rule>(alpha);
  }
}

void DistributedRuleCounts::AddNonterminal(
    vector<RuleCounts>& rule_counts, int nonterminal) {
  for (auto& restaurant: rule_counts) {
    if (!restaurant.count(nonterminal)) {
        restaurant[nonterminal] = RestaurantProcess<Rule>(alpha);
    }
  }
}

void DistributedRuleCounts::Increment(const Rule& rule) {
  int thread_id = omp_get_thread_num();
  int root_tag = rule.first.GetRootTag();
  rule_counts[thread_id][root_tag].Update(rule, 1);
}

void DistributedRuleCounts::Decrement(const Rule& rule) {
  int thread_id = omp_get_thread_num();
  int root_tag = rule.first.GetRootTag();
  rule_counts[thread_id][root_tag].Update(rule, -1);
}

double DistributedRuleCounts::GetLogProbability(const Rule& rule, double p0) {
  int thread_id = omp_get_thread_num();
  int root_tag = rule.first.GetRootTag();
  return rule_counts[thread_id][root_tag].GetLogProbability(rule, p0);
}

double DistributedRuleCounts::GetLogProbability(
    const Rule& rule, int same_rules, int same_tags, double p0) {
  int thread_id = omp_get_thread_num();
  int root_tag = rule.first.GetRootTag();
  return rule_counts[thread_id][root_tag].GetLogProbability(
      rule, same_rules, same_tags, p0);
}

vector<int> DistributedRuleCounts::GetNonterminals() {
  vector<int> nonterminals;
  int thread_id = omp_get_thread_num();
  for (const auto& entry: rule_counts[thread_id]) {
    nonterminals.push_back(entry.first);
  }
  return nonterminals;
}

void DistributedRuleCounts::UpdateCounts(
    RuleCounts& total_counts,
    const RuleCounts& restaurant,
    int factor) {
  for (const auto& entry: restaurant) {
    int root_tag = entry.first;
    if (!total_counts.count(root_tag)) {
      total_counts[root_tag] = RestaurantProcess<Rule>(alpha);
    }

    const auto& rule_counts = entry.second.Get();
    for (const auto& rule_entry: rule_counts) {
      total_counts[root_tag].Update(
          rule_entry.first, factor * rule_entry.second);
    }
  }
}

void DistributedRuleCounts::UpdateCounts(
    RuleCounts& total_counts,
    const vector<RuleCounts>& rule_counts,
    int factor) {
  for (const auto& restaurant: rule_counts) {
    UpdateCounts(total_counts, restaurant, factor);
  }
}

int DistributedRuleCounts::Count(const Rule& rule) const {
  int thread_id = omp_get_thread_num();
  int root_tag = rule.first.GetRootTag();
  return rule_counts[thread_id].at(root_tag).Count(rule);
}

int DistributedRuleCounts::Count(int nonterminal) const {
  int thread_id = omp_get_thread_num();
  return rule_counts[thread_id].at(nonterminal).GetTotal();
}

void DistributedRuleCounts::Synchronize() {
  cerr << "Synchronizing..." << endl;
  Clock::time_point start_time = Clock::now();

  RuleCounts total_counts;
  UpdateCounts(total_counts, rule_counts, 1);
  UpdateCounts(total_counts, snapshot, -rule_counts.size());
  UpdateCounts(snapshot, total_counts, 1);

  for (size_t i = 0; i < rule_counts.size(); ++i) {
    rule_counts[i] = snapshot;
  }

  Clock::time_point end_time = Clock::now();
  cerr << "Synchronization took "
       << duration_cast<milliseconds>(end_time - start_time).count() / 1000.0
       << " seconds" << endl;
}
