#include "rule_stats_reporter.h"

#include "dictionary.h"
#include "aligned_tree.h"

void RuleStatsReporter::UpdateRuleStats(const pair<Rule, double>& rule) {
  if (IsReorderingRule(rule.first)) {
    #pragma omp critical
    {
      ++rule_counts[rule.first];
      rule_probs[rule.first] = rule.second;
    }
  }
}

void RuleStatsReporter::DisplayRuleStats(
    ostream& stream, Dictionary& dictionary) {
  vector<pair<int, Rule>> top_rules;
  for (auto rule: rule_counts) {
    top_rules.push_back(make_pair(rule.second, rule.first));
  }

  sort(top_rules.begin(), top_rules.end(), greater<pair<int, Rule>>());

  for (auto rule: top_rules) {
    WriteSTSGRule(stream, rule.second, dictionary);
    stream << " ||| " << rule_probs[rule.second] << " ||| "
           << rule.first << endl;
  }
}

bool RuleStatsReporter::IsReorderingRule(const Rule& rule) {
  const AlignedTree& tree = rule.first;
  const String& reordering = rule.second;
  int target_index = 0, var_index = 0;
  for (auto leaf = tree.begin_leaf(); leaf != tree.end_leaf(); ++leaf) {
    if (leaf->IsSetWord() ^ reordering[target_index].IsSetWord()) {
      return true;
    }

    if (leaf->IsSetWord() && reordering[target_index].IsSetWord() &&
        leaf->GetWord() != reordering[target_index].GetWord()) {
      return true;
    }

    if (!leaf->IsSetWord() && !reordering[target_index].IsSetWord() &&
        var_index != reordering[target_index].GetVarIndex()) {
      return true;
    }

    var_index += !leaf->IsSetWord();
    ++target_index;
  }

  return false;
}
