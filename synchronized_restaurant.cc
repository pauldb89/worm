#include "synchronized_restaurant.h"

#include "aligned_tree.h"

SynchronizedRuleCounts::SynchronizedRuleCounts(double alpha) :
    alpha(alpha) {}

void SynchronizedRuleCounts::AddNonterminal(int nonterminal) {
  lock_guard<mutex> general_lock(general_mutex);
  if (!counts.count(nonterminal)) {
    counts[nonterminal] = RuleCounts(alpha, false);
    mutexes[nonterminal] = make_shared<boost::shared_mutex>();
  }
}

void SynchronizedRuleCounts::Increment(const Rule& rule) {
  int root_tag = rule.first.GetRootTag();
  boost::upgrade_lock<boost::shared_mutex> lock(*mutexes[root_tag]);
  boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(lock);
  counts[root_tag].increment(rule);
}

void SynchronizedRuleCounts::Decrement(const Rule& rule) {
  int root_tag = rule.first.GetRootTag();
  boost::upgrade_lock<boost::shared_mutex> lock(*mutexes[root_tag]);
  boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(lock);
  counts[root_tag].decrement(rule);
}

double SynchronizedRuleCounts::GetLogProbability(const Rule& rule, double p0) {
  int root_tag = rule.first.GetRootTag();
  boost::shared_lock<boost::shared_mutex> lock(*mutexes[root_tag]);
  return counts[root_tag].log_prob(rule, p0);
}

double SynchronizedRuleCounts::GetLogProbability(
    const Rule& rule, int same_rules, int same_tags, double p0) {
  int root_tag = rule.first.GetRootTag();
  boost::shared_lock<boost::shared_mutex> lock(*mutexes[root_tag]);
  return counts[root_tag].log_prob(rule, same_rules, same_tags, p0);
}

vector<int> SynchronizedRuleCounts::GetNonterminals() {
  vector<int> nonterminals;
  lock_guard<mutex> general_lock(general_mutex);
  for (const auto& entry: counts) {
    nonterminals.push_back(entry.first);
  }
  return nonterminals;
}
