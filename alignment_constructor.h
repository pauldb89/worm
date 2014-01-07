#pragma once

#include <memory>

#include "definitions.h"

using namespace std;

class TranslationTable;

class AlignmentConstructor {
 public:
  AlignmentConstructor(
      shared_ptr<TranslationTable> forward_table,
      shared_ptr<TranslationTable> reverse_table);

  pair<Alignment, Alignment> ConstructAlignments(const Rule& rule);

  Alignment ConstructNonterminalLinks(const Rule& rule);

  pair<Alignment, Alignment> ConstructTerminalLinks(const Rule& rule);

 private:
  shared_ptr<TranslationTable> forward_table, reverse_table;
};
