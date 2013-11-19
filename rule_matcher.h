#pragma once

#include <map>
#include <vector>

#include "grammar.h"
#include "util.h"

using namespace std;

typedef vector<pair<pair<Rule, double>, vector<NodeIter>>> MatchingRules;

class RuleMatcher {
 public:
  RuleMatcher(const Grammar& grammar, const AlignedTree& tree);

  MatchingRules GetRules(const NodeIter& node) const;

 private:
  bool MatchRule(const AlignedTree& tree, const NodeIter& tree_node,
                 const AlignedTree& frag, const NodeIter& frag_node,
                 vector<NodeIter>& frontier) const;

  map<NodeIter, MatchingRules> matcher;
};
