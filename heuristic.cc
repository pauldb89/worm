#include <boost/program_options.hpp>

#include "aligned_tree.h"
#include "alignment_heuristic.h"
#include "dictionary.h"
#include "time_util.h"
#include "translation_table.h"
#include "util.h"

using namespace std;
namespace po = boost::program_options;

int main(int argc, char** argv) {
  po::options_description cmdline_specific("Command line options");
  cmdline_specific.add_options()
      ("help,h", "Show available options")
      ("config,c", po::value<string>(), "Path to config file");

  po::options_description general_options("General options");
  general_options.add_options()
      ("gdfa", po::value<string>()->required(),
          "File containing GDFA alignments")
      ("intersect", po::value<string>()->required(),
          "File containing intersect alignments")
      ("trees,t", po::value<string>()->required(),
          "File containing source parse trees in .ptb format")
      ("strings,s", po::value<string>()->required(),
          "File containing target strings")
      ("output,o", po::value<string>()->required(),
          "Output file for writing the best alignments")
      ("threads", po::value<int>()->default_value(1)->required(),
          "Number of threads to use for finding the best alignments")
      ("max_links", po::value<unsigned int>()->default_value(10)->required(),
          "Maximum number of links to backtrack when searching for the best "
          "derivation")
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

  int num_threads = vm["threads"].as<int>();
  cerr << "Applying alignment heuristic using " << num_threads
       << " threads..." << endl;

  Dictionary dictionary;
  shared_ptr<TranslationTable> forward_table, reverse_table;
  LoadTranslationTables(vm, forward_table, reverse_table, dictionary);

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

  cerr << "Reading GDFA alignments..." << endl;
  vector<Alignment> gdfa_alignments;
  ifstream gdfa_stream(vm["gdfa"].as<string>());
  while (!gdfa_stream.eof()) {
    Alignment alignment;
    gdfa_stream >> alignment >> ws;
    gdfa_alignments.push_back(alignment);
  }
  cerr << "Done..." << endl;

  cerr << "Reading intersect alignments..." << endl;
  vector<Alignment> intersect_alignments;
  ifstream intersect_stream(vm["intersect"].as<string>());
  while (!intersect_stream.eof()) {
    Alignment alignment;
    intersect_stream >> alignment >> ws;
    intersect_alignments.push_back(alignment);
  }
  cerr << "Done..." << endl;

  assert(parse_trees.size() == target_strings.size() &&
         parse_trees.size() == gdfa_alignments.size() &&
         parse_trees.size() == intersect_alignments.size());

  unordered_set<int> blacklisted_tags;
  for (const string& tag: {"IN", "DT", "CC"}) {
    blacklisted_tags.insert(dictionary.GetIndex(tag));
  }
  AlignmentHeuristic heuristic(
      forward_table, reverse_table, blacklisted_tags,
      vm["max_links"].as<unsigned int>());
  auto start_time = GetTime();
  vector<Alignment> best_alignments(parse_trees.size());
  int alignment_index = 0;
  #pragma omp parallel for schedule(dynamic) num_threads(num_threads)
  for (size_t i = 0; i < best_alignments.size(); ++i) {
    best_alignments[i] = heuristic.FindBestAlignment(
        parse_trees[i], target_strings[i], gdfa_alignments[i],
        intersect_alignments[i]);
    #pragma omp critical
    {
      ++alignment_index;
      if (alignment_index % 1000 == 0) {
        cerr << ".";
        if (alignment_index % 50000 == 0) {
          cerr << " [" << alignment_index << "]" << endl;
        }
      }
    }
  }
  auto end_time = GetTime();
  cerr << "Done..." << endl;
  cerr << "Applying alignment heuristic took "
       << GetDuration(start_time, end_time) << " seconds..." << endl;

  ofstream output_stream(vm["output"].as<string>());
  for (const auto& alignment: best_alignments) {
    output_stream << alignment << endl;
  }

  return 0;
}
