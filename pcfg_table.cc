#include "pcfg_table.h"

#include "aligned_tree.h"
#include "log_add.h"
#include "node.h"

PCFGTable::PCFGTable(const shared_ptr<vector<Instance>>& training) {
  // Count CFG rules.
  for (auto instance: *training) {
    const AlignedTree& tree = instance.first;
    for (auto node = tree.begin(); node != tree.end(); ++node) {
      vector<int> rhs;
      if (node.number_of_children() > 0) {
        for (auto child = tree.begin(node); child != tree.end(node); ++child) {
          rhs.push_back(child->GetTag());
        }
      } else {
        rhs.push_back(node->GetWord());
      }

      rule_probs[node->GetTag()][rhs] += 1;
    }
  }

  // Normalize counts to compute log probabilities.
  for (auto &lhs_entry: rule_probs) {
    double total = 0;
    for (auto rhs_prob_pair: lhs_entry.second) {
      total += rhs_prob_pair.second;
    }

    for (auto &rhs_prob_pair: lhs_entry.second) {
      rhs_prob_pair.second = log(rhs_prob_pair.second / total);
    }
  }
}

double PCFGTable::GetLogProbability(int lhs, const vector<int>& rhs) {
  auto lhs_result = rule_probs.find(lhs);
  if (lhs_result != rule_probs.end()) {
    auto rhs_result = lhs_result->second.find(rhs);
    if (rhs_result != lhs_result->second.end()) {
      return rhs_result->second;
    }
  }

  assert(false);
  return Log<double>::zero();
}
