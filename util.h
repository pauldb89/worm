#ifndef _UTIL_H_
#define _UTIL_H_

#include <fstream>
#include <vector>

using namespace std;

class AlignedTree;
class Dictionary;
class StringNode;

typedef vector<pair<int, int>> Alignment;
typedef vector<StringNode> String;
typedef pair<AlignedTree, String> Instance;
typedef pair<AlignedTree, String> Rule;

// Reads a training instance from input files.
Instance ReadInstance(istream& tree_stream,
                      istream& string_stream,
                      istream& alignment_stream,
                      Dictionary& dictionary);

// Reads a parse tree from a file in ptb format.
AlignedTree ReadParseTree(istream& tree_stream, Dictionary& dictionary);

// Reads a target sentence from file.
String ReadTargetString(istream& string_stream, Dictionary& dictionary);

pair<Rule, double> ReadRule(istream& grammar_stream, Dictionary& dictionary);

istream& operator>>(istream& in, Alignment& alignment);

void ConstructGHKMDerivation(AlignedTree& tree,
                             const String& target_string,
                             istream& alignment_stream,
                             Dictionary& dictionary);

void WriteTargetString(ostream& out,
                       const String& target_string,
                       Dictionary& dictionary);

void WriteSCFGRule(ostream& out, const Rule& rule, Dictionary& dictionary);

void WriteSTSGRule(ostream& out, const Rule& rule, Dictionary& dictionary);

ostream& operator<<(ostream& out, const Alignment& alignment);

#endif
