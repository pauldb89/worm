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
Instance ReadInstance(ifstream& tree_stream,
                      ifstream& string_stream,
                      ifstream& alignment_stream,
                      Dictionary& dictionary);

// Reads a parse tree from a file in ptb format.
AlignedTree ReadParseTree(ifstream& tree_stream, Dictionary& dictionary);

// Reads a target sentence from file.
String ReadTargetString(ifstream& string_stream, Dictionary& dictionary);

void ConstructGHKMDerivation(AlignedTree& tree,
                             const String& target_string,
                             ifstream& alignment_stream,
                             Dictionary& dictionary);

void WriteTargetString(ofstream& out,
                       const String& target_string,
                       Dictionary& dictionary);

void WriteSCFGRule(ofstream& out, const Rule& rule, Dictionary& dictionary);

void WriteSTSGRule(ofstream& out, const Rule& rule, Dictionary& dictionary);

ofstream& operator<<(ofstream& out, const Alignment& alignment);

#endif
