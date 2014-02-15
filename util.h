#pragma once

#include <fstream>
#include <memory>
#include <vector>

#include <boost/program_options.hpp>

#include "definitions.h"

using namespace std;
namespace po = boost::program_options;

class Dictionary;
class TranslationTable;

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

void ReadInternalStructure(
    istream& in, AlignedTree& tree, Dictionary& dictionary, int tree_index);

void ConstructGHKMDerivation(AlignedTree& tree,
                             const String& target_string,
                             const Alignment& alignment);

void WriteTargetString(ostream& out,
                       const String& target_string,
                       Dictionary& dictionary);

void WriteSCFGRule(ostream& out, const Rule& rule, Dictionary& dictionary);

void WriteSTSGRule(ostream& out, const Rule& rule, Dictionary& dictionary);

ostream& operator<<(ostream& out, const Alignment& alignment);

string GetOutputFilename(
  const string& output_directory,
  const string& extension,
  const string& iteration = "");

void LoadTranslationTables(
    po::variables_map vm,
    shared_ptr<TranslationTable>& forward_table,
    shared_ptr<TranslationTable>& reverse_table,
    Dictionary& Dictionary);

vector<Instance> LoadInternalState(
    po::variables_map vm, Dictionary& dictionary);
