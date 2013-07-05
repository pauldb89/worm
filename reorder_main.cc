#include <chrono>
#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include "aligned_tree.h"
#include "dictionary.h"
#include "grammar.h"
#include "single_sample_reorderer.h"
#include "viterbi_reorderer.h"

using namespace std;
using namespace chrono;
namespace po = boost::program_options;

typedef high_resolution_clock Clock;

int main(int argc, char** argv) {
  po::options_description cmdline_specific("Command line options");
  cmdline_specific.add_options()
      ("help", "Show available options")
      ("config", po::value<string>(), "Path to config file");

  po::options_description general_options("General options");
  general_options.add_options()
      ("grammar,g", po::value<string>()->required(), "Path to grammar file")
      ("alignment,a", po::value<string>(),
          "Path to file containing rule alignments")
      ("threads", po::value<int>()->default_value(1)->required(),
          "Number of threads for reordering")
      ("penalty", po::value<double>()->default_value(0.1)->required(),
          "Displacement penalty for reordering")
      ("max_leaves", po::value<int>()->default_value(5)->required(),
          "Maximum number of leaves in rules that are reordered")
      ("max_tree_size", po::value<int>()->default_value(8)->required(),
          "Maximum size of a tree rule that is reordered")
      ("stats_file", po::value<string>(),
          "Target file for writing stats about the reordering grammar");

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

  cerr << "Constructing reordering grammar..." << endl;
  Dictionary dictionary;
  shared_ptr<Grammar> grammar;
  ifstream grammar_stream(vm["grammar"].as<string>());
  if (vm.count("alignment")) {
    ifstream alignment_stream(vm["alignment"].as<string>());
    grammar = make_shared<Grammar>(grammar_stream, alignment_stream, dictionary,
        vm["penalty"].as<double>(), vm["max_leaves"].as<int>(),
        vm["max_tree_size"].as<int>());
  } else {
    grammar = make_shared<Grammar>(grammar_stream, dictionary,
        vm["penalty"].as<double>(), vm["max_leaves"].as<int>(),
        vm["max_tree_size"].as<int>());
  }
  cerr << "Done..." << endl;

  cerr << "Reading training data..." << endl;
  vector<AlignedTree> input_trees;
  while (cin.good()) {
    input_trees.push_back(ReadParseTree(cin, dictionary));
    cin >> ws;
  }
  cerr << "Done..." << endl;

  int sentence_index = 0;
  Clock::time_point start_time = Clock::now();
  vector<String> reorderings(input_trees.size());
  int num_threads = vm["threads"].as<int>();
  cerr << "Reordering will use " << num_threads << " threads." << endl;
  #pragma omp parallel for schedule(dynamic) num_threads(num_threads)
  for (size_t i = 0; i < input_trees.size(); ++i) {
    ViterbiReorderer reorderer(grammar);

    // Ignore unparsable sentences.
    if (input_trees[i].size() <= 1) {
      reorderings[i] = String();
    } else {
      reorderings[i] = reorderer.Reorder(input_trees[i]);
    }

    #pragma omp critical
    {
      ++sentence_index;
      if (sentence_index % 10 == 0) {
        cerr << ".";
        if (sentence_index % 1000 == 0) {
          cerr << " [" << sentence_index << "]" << endl;
        }
      }
    }
  }
  Clock::time_point stop_time = Clock::now();
  auto duration = duration_cast<milliseconds>(stop_time - start_time).count();
  cerr << endl << "Reordering " << reorderings.size() << " sentences took "
       << duration / 1000.0 << " seconds..." << endl;

  cerr << "Writing reordered sentences..." << endl;
  for (String reordering: reorderings) {
    WriteTargetString(cout, reordering, dictionary);
    cout << "\n";
  }
  cerr << "Done..." << endl;

  if (vm.count("stats_file")) {
    cerr << "Writing grammar statistics..." << endl;
    ofstream stats_stream(vm["stats_file"].as<string>());
    grammar->DisplayRuleStats(stats_stream, dictionary, reorderings.size());
    cerr << "Done..." << endl;
  }

  return 0;
}
