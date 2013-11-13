#include "synchronized_restaurant.h"

#include <omp.h>

#include "aligned_tree.h"

SynchronizedRuleCounts::SynchronizedRuleCounts(double alpha) :
    alpha(alpha) {}

void SynchronizedRuleCounts::AddNonterminal(int nonterminal) {
  lock_guard<mutex> guard(general_lock);
  if (!counts.count(nonterminal)) {
    counts[nonterminal] = RuleCounts(alpha, false);
    locks[nonterminal] = shared_ptr<mutex>(new mutex());
  }
}

void SynchronizedRuleCounts::Increment(const Rule& rule) {
  int root_tag = rule.first.GetRootTag();
  lock_guard<mutex> guard(*locks[root_tag]);
  counts[root_tag].increment(rule);
}

void SynchronizedRuleCounts::Decrement(const Rule& rule) {
  int root_tag = rule.first.GetRootTag();
  lock_guard<mutex> guard(*locks[root_tag]);
  counts[root_tag].decrement(rule);
}

double SynchronizedRuleCounts::GetLogProbability(const Rule& rule, double p0) {
  int root_tag = rule.first.GetRootTag();
  lock_guard<mutex> guard(*locks[root_tag]);
  return counts[root_tag].log_prob(rule, p0);
}

double SynchronizedRuleCounts::GetLogProbability(
    const Rule& rule, int same_rules, int same_tags, double p0) {
  int root_tag = rule.first.GetRootTag();
  lock_guard<mutex> guard(*locks[root_tag]);
  return counts[root_tag].log_prob(rule, same_rules, same_tags, p0);
}

vector<int> SynchronizedRuleCounts::GetNonterminals() {
  vector<int> nonterminals;
  lock_guard<mutex> guard(general_lock);
  for (const auto& entry: counts) {
    nonterminals.push_back(entry.first);
  }
  return nonterminals;
}
