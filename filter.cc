#include <fstream>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "aligned_tree.h"
#include "alignment_constructor.h"
#include "dictionary.h"
#include "distributed_rule_counts.h"
#include "rule_extractor.h"
#include "translation_table.h"
#include "util.h"

using namespace std;
namespace fs = boost::filesystem;
namespace po = boost::program_options;

int main(int argc, char** argv) {
  po::options_description cmdline_specific("Command line options");
  cmdline_specific.add_options()
      ("help,h", "Show available options")
      ("config,c", po::value<string>(), "Path to config file");

  po::options_description general_options;
  general_options.add_options()
      ("trees,t", po::value<string>()->required(),
          "File containing source parse trees in .ptb format")
      ("strings,s", po::value<string>()->required(),
          "File containing target strings")
      ("internal,i", po::value<string>()->required(),
          "File containing hidden alignment variables")
      ("alpha", po::value<double>()->required(),
          "Dirichlet process concentration parameter")
      ("threshold", po::value<int>()->required(),
          "Minimum number of occurrences for a rule")
      ("output", po::value<string>()->required(), "Output directory")
      ("ibm1-source-vcb", po::value<string>()->required(),
          "Giza++ source vocabulary file")
      ("ibm1-target-vcb", po::value<string>()->required(),
          "Giza++ target vocabulary file")
      ("ibm1-forward", po::value<string>()->required(),
          "Path to the IBM Model 1 translation table p(t|s). Expected format: "
          "source_word_id target_word_id probability.")
      ("ibm1-reverse", po::value<string>()->required(),
          "Path to the IBM Model 1 translation table p(s|t). Expected format: "
          "target_word_id source_word_id probability.");

  po::variables_map vm;
  po::options_description cmdline_options;
  cmdline_options.add(cmdline_specific).add(general_options);
  po::store(po::parse_command_line(argc, argv, cmdline_options), vm);

  if (vm.count("help")) {
    cout << cmdline_options << endl;
    return 0;
  }

  if (vm.count("config")) {
    po::options_description config_options;
    config_options.add(general_options);
    ifstream config_stream(vm["config"].as<string>());
    po::store(po::parse_config_file(config_stream, config_options), vm);
  }

  po::notify(vm);

  Dictionary dictionary;
  cerr << "Reading monolingual dictionaries..." << endl;
  shared_ptr<TranslationTable> forward_table, reverse_table;
  ifstream source_vcb_stream(vm["ibm1-source-vcb"].as<string>());
  Dictionary source_vocabulary(source_vcb_stream);
  ifstream target_vcb_stream(vm["ibm1-target-vcb"].as<string>());
  Dictionary target_vocabulary(target_vcb_stream);
  cerr << "Done..." << endl;

  cerr << "Reading translation tables..." << endl;
  ifstream forward_stream(vm["ibm1-forward"].as<string>());
  forward_table = make_shared<TranslationTable>(
      forward_stream, source_vocabulary, target_vocabulary, dictionary, 1);
  ifstream reverse_stream(vm["ibm1-reverse"].as<string>());
  reverse_table = make_shared<TranslationTable>(
      reverse_stream, target_vocabulary, source_vocabulary, dictionary, 1);
  cerr << "Done..." << endl;


  cerr << "Reading parse trees..." << endl;
  vector<AlignedTree> parse_trees;
  ifstream tree_stream(vm["trees"].as<string>());
  while (!tree_stream.eof()) {
    parse_trees.push_back(ReadParseTree(tree_stream, dictionary));
    tree_stream >> ws;
  }
  cerr << "Done..." << endl;

  cerr << "Reading target strings..." << endl;
  vector<String> target_strings;
  ifstream string_stream(vm["strings"].as<string>());
  while (!string_stream.eof()) {
    target_strings.push_back(ReadTargetString(string_stream, dictionary));
    string_stream >> ws;
  }
  cerr << "Done..." << endl;

  cerr << "Reading internal structure..." << endl;
  ifstream internal_stream(vm["internal"].as<string>());
  for (size_t i = 0; i < parse_trees.size(); ++i) {
    ReadInternalStructure(internal_stream, parse_trees[i], dictionary, i);
    internal_stream >> ws;
  }
  cerr << "Done..." << endl;

  assert(parse_trees.size() == target_strings.size());
  vector<Instance> training;
  for (size_t i = 0; i < parse_trees.size(); ++i) {
    training.push_back(make_pair(parse_trees[i], target_strings[i]));
  }

  unordered_map<int, set<Rule>> rules;
  RuleExtractor extractor;
  DistributedRuleCounts rule_counts(1, vm["alpha"].as<double>());
  for (size_t i = 0; i < training.size(); ++i) {
    const Instance& instance = training[i];
    const AlignedTree& tree = instance.first;
    for (auto node = tree.begin(); node != tree.end(); ++node) {
      if (node->IsSplitNode()) {
        const Rule& rule = extractor.ExtractRule(instance, node);
        int root_tag = rule.first.GetRootTag();
        rules[root_tag].insert(rule);
        rule_counts.Increment(rule);
      }
    }
  }

  int threshold = vm["threshold"].as<int>();
  string output_directory = vm["output"].as<string>();
  fs::path output_path(output_directory);
  if (!fs::exists(output_path)) {
    fs::create_directory(output_path);
  }
  if (output_directory.back() != '/') {
    output_directory = output_directory + "/";
  }

  AlignmentConstructor alignment_constructor(forward_table, reverse_table);
  ofstream gout(GetOutputFilename(output_directory, "grammar"));
  ofstream fwd_out(GetOutputFilename(output_directory, "fwd"));
  ofstream rev_out(GetOutputFilename(output_directory, "rev"));
  for (const auto& entry: rules) {
    double total_count = 0;
    vector<Rule> frequent_rules;
    for (const Rule& rule: entry.second) {
      int rule_count = rule_counts.Count(rule);
      if (rule_count >= threshold) {
        frequent_rules.push_back(rule);
        total_count += rule_count;
      }
    }

    for (const Rule& rule: frequent_rules) {
      double rule_prob = rule_counts.Count(rule) / total_count;
      WriteSTSGRule(gout, rule, dictionary);
      gout << " ||| " << rule_prob << "\n";

      auto alignments = alignment_constructor.ConstructAlignments(rule);
      fwd_out << alignments.first << "\n";
      rev_out << alignments.second << "\n";
    }
  }

  return 0;
}
