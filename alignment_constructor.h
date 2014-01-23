#pragma once

#include <memory>

#include "definitions.h"
#include "rule_extractor.h"

using namespace std;

class TranslationTable;

class AlignmentConstructor {
 public:
  AlignmentConstructor(
      shared_ptr<TranslationTable> forward_table,
      shared_ptr<TranslationTable> reverse_table);

  pair<Alignment, Alignment> ExtractAlignments(const Instance& instance);

  pair<Alignment, Alignment> ConstructAlignments(const Rule& rule);

  Alignment ConstructNonterminalLinks(const Rule& rule);

  pair<Alignment, Alignment> ConstructTerminalLinks(const Rule& rule);

 private:
  RuleExtractor extractor;
  shared_ptr<TranslationTable> forward_table, reverse_table;
};
