#ifndef _UTIL_H_
#define _UTIL_H_

#include <fstream>
#include <memory>
#include <vector>

#include "definitions.h"

using namespace std;

class Dictionary;

Instance ConstructInstance(
    const AlignedTree& parse_tree,
    const String& target_string,
    const Alignment& alignment);

// Reads a parse tree from a file in ptb format.
AlignedTree ReadParseTree(istream& tree_stream, Dictionary& dictionary);

// Reads a target sentence from file.
String ReadTargetString(istream& string_stream, Dictionary& dictionary);

pair<Rule, double> ReadRule(istream& grammar_stream, Dictionary& dictionary);

istream& operator>>(istream& in, Alignment& alignment);

void ConstructGHKMDerivation(AlignedTree& tree,
                             const String& target_string,
                             const Alignment& alignment);

void WriteTargetString(ostream& out,
                       const String& target_string,
                       Dictionary& dictionary);

void WriteSCFGRule(ostream& out, const Rule& rule, Dictionary& dictionary);

void WriteSTSGRule(ostream& out, const Rule& rule, Dictionary& dictionary);

ostream& operator<<(ostream& out, const Alignment& alignment);

#endif
