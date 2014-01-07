#pragma once

#include "aligned_tree.h"
#include "definitions.h"

class RuleExtractor {
 public:
  Rule ExtractRule(const Instance& instance, const NodeIter& node) const;

 private:
  String ConstructRuleTargetSide(
      const AlignedTree& fragment, const String& target_string) const;
};
