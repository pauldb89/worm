#ifndef _UTIL_H_
#define _UTIL_H_

#include <fstream>
#include <vector>

using namespace std;

class AlignedTree;
class Dictionary;
class StringNode;

typedef vector<StringNode> String;
typedef pair<AlignedTree, String> Instance;

// Reads a training instance from input files.
Instance ReadInstance(ifstream& tree_stream,
                      ifstream& string_stream,
                      Dictionary& dictionary);

// Reads a parse tree from a file in ptb format.
AlignedTree ReadParseTree(ifstream& tree_stream, Dictionary& dictionary);

// Reads a target sentence from file.
String ReadString(ifstream& string_stream, Dictionary& dictionary);

#endif
