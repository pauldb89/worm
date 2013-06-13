#include <chrono>
#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include "aligned_tree.h"
#include "dictionary.h"
#include "grammar.h"
#include "viterbi_reorderer.h"

using namespace std;
using namespace chrono;
namespace po = boost::program_options;

typedef high_resolution_clock Clock;

int main(int argc, char** argv) {
  po::options_description desc("Command line options");

  desc.add_options()
      ("help", "Show available options")
      ("grammar,g", po::value<string>()->required(), "Path to grammar file")
      ("alignment,a", po::value<string>()->required(),
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
  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("help")) {
    cout << desc << endl;
    return 0;
  }

  po::notify(vm);

  cerr << "Constructing reordering grammar..." << endl;
  Dictionary dictionary;
  ifstream grammar_stream(vm["grammar"].as<string>());
  ifstream alignment_stream(vm["alignment"].as<string>());
  shared_ptr<Grammar> grammar = make_shared<Grammar>(
      grammar_stream, alignment_stream, dictionary, vm["penalty"].as<double>(),
      vm["max_leaves"].as<int>(), vm["max_tree_size"].as<int>());
  cerr << "Done..." << endl;

  vector<AlignedTree> input_trees;
  while (cin.good()) {
    input_trees.push_back(ReadParseTree(cin, dictionary));
    cin >> ws;
  }

  int sentence_index = 0;
  Clock::time_point start_time = Clock::now();
  vector<String> reorderings(input_trees.size());
  int num_threads = vm["threads"].as<int>();
  cerr << "Reordering will use " << num_threads << " threads." << endl;
  #pragma omp parallel for schedule(dynamic) num_threads(num_threads)
  for (size_t i = 0; i < input_trees.size(); ++i) {
    ViterbiReorderer reorderer(grammar, dictionary);

    // Ignore unparsable sentences.
    if (input_trees[i].size() <= 1) {
      reorderings[i] = String();
    } else {
      reorderings[i] = reorderer.Reorder(input_trees[i]);
    }

    #pragma omp critical
    {
      ++sentence_index;
      cerr << ".";
      if (sentence_index % 100 == 0) {
        cerr << " [" << sentence_index << "]" << endl;
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
    grammar->DisplayRuleStats(stats_stream, dictionary);
    cerr << "Done..." << endl;
  }

  return 0;
}
