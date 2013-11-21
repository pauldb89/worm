#include <chrono>
#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include "aligned_tree.h"
#include "dictionary.h"
#include "grammar.h"
#include "multi_sample_reorderer.h"
#include "rule_stats_reporter.h"
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
      ("alignment,a", po::value<string>()->required(),
          "Path to file containing rule alignments")
      ("threads", po::value<int>()->default_value(1)->required(),
          "Number of threads for reordering")
      ("iterations", po::value<unsigned int>()->default_value(0),
          "Number of samples to determine the reordering for each parse tree. "
          "If not set, a max-derivation reorderer will be used instead.")
      ("threshold", po::value<double>()->default_value(0)->required(),
          "Minimum probabilty for reodering rules")
      ("seed", po::value<unsigned int>()->default_value(0),
          "Seed for random generator. Set to 0 if seed should be random.")
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
  ifstream grammar_stream(vm["grammar"].as<string>());
  ifstream alignment_stream(vm["alignment"].as<string>());
  Grammar grammar(grammar_stream, alignment_stream, dictionary,
      vm["penalty"].as<double>(), vm["threshold"].as<double>(),
      vm["max_leaves"].as<int>(), vm["max_tree_size"].as<int>());
  cerr << "Done..." << endl;

  cerr << "Reading training data..." << endl;
  vector<AlignedTree> input_trees;
  while (cin.good()) {
    input_trees.push_back(ReadParseTree(cin, dictionary));
    cin >> ws;
  }
  cerr << "Done..." << endl;

  unsigned int num_iterations = 0;
  if (vm.count("iterations")) {
    num_iterations = vm["iterations"].as<unsigned int>();
  }
  if (num_iterations > 0) {
    cerr << "Using sampling-based reordering with " << num_iterations
         << " iterations..." << endl;
  } else {
    cerr << "Using max-derivation (Viterbi) reorderer..." << endl;
  }

  int sentence_index = 0;
  Clock::time_point start_time = Clock::now();
  shared_ptr<RuleStatsReporter> reporter = make_shared<RuleStatsReporter>();
  vector<String> reorderings(input_trees.size());
  int num_threads = vm["threads"].as<int>();
  cerr << "Reordering will use " << num_threads << " threads." << endl;
  #pragma omp parallel for schedule(dynamic) num_threads(num_threads)
  for (size_t i = 0; i < input_trees.size(); ++i) {
    shared_ptr<ReordererBase> reorderer;
    if (num_iterations) {
      unsigned int seed = vm["seed"].as<unsigned int>();
      if (seed == 0) {
        seed = time(NULL);
      }
      RandomGenerator generator(seed);
      reorderer = make_shared<MultiSampleReorderer>(
          input_trees[i], grammar, reporter, generator, num_iterations);
    } else {
      reorderer = make_shared<ViterbiReorderer>(
          input_trees[i], grammar, reporter);
    }

    Clock::time_point reordering_start = Clock::now();
    // Ignore unparsable sentences.
    if (input_trees[i].size() <= 1) {
      reorderings[i] = String();
    } else {
      reorderings[i] = reorderer->ConstructReordering();
    }
    Clock::time_point reordering_end = Clock::now();
    double reordering_duration = duration_cast<milliseconds>(
        reordering_end - reordering_start).count() / 1000.0;
    cerr << "Time required to reorder sentence: " << reordering_duration
         << " seconds..." << endl;

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
    reporter->DisplayRuleStats(stats_stream, dictionary);
    cerr << "Done..." << endl;
  }

  return 0;
}
