#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "aligned_tree.h"
#include "dictionary.h"
#include "pcfg_table.h"
#include "sampler.h"
#include "translation_table.h"
#include "util.h"

using namespace std;
namespace po = boost::program_options;

int main(int argc, char **argv) {
  po::options_description cmdline_specific("Command line options");
  cmdline_specific.add_options()
      ("help,h", "Show available options")
      ("config,c", po::value<string>(), "Path to config file");

  po::options_description general_options("General options");
  general_options.add_options()
      ("trees,t", po::value<string>()->required(),
          "File containing source parse trees in .ptb format")
      ("strings,s", po::value<string>()->required(),
          "File containing target strings")
      ("alignment,a", po::value<string>()->required(),
          "File containing word alignments for GHKM")
      ("output,o", po::value<string>()->required(), "Output prefix")
      ("threads", po::value<int>()->default_value(1)->required(),
          "Number of threads to use for sampling")
      ("align", "Infer alignments instead of a STSG grammar")
      ("reorder", "Infer reordering directly from sampled variables")
      ("smart_expand", "Use smart expansion probabilities")
      ("stats", "Display statistics about the grammar after each iteration")
      ("scfg", "Print grammar as SCFG instead of STSG")
      ("penalty", po::value<double>()->default_value(0.1)->required(),
          "Displacement penalty for reordering")
      ("max_leaves", po::value<int>()->default_value(5)->required(),
          "Maximum number of leaves in rules used for inferring reorderings")
      ("max_tree_size", po::value<int>()->default_value(8)->required(),
          "Maximum size of a tree in rules used for inferring reorderings")
      ("min_rule_count", po::value<int>()->default_value(0)->required(),
               "Minimum count for a rule to be used for reordering")
      ("alpha", po::value<double>()->default_value(1.0)->required(),
          "Dirichlet process concentration parameter")
      ("iterations", po::value<int>()->default_value(100)->required(),
          "Number of iterations")
      ("log_freq", po::value<int>()->default_value(0)->required(),
          "Frequency (in iterations) for serializing the grammar to disk")
      ("pexpand", po::value<double>()->default_value(0.5)->required(),
          "Param. for the Bernoulli distr. for a node to be split")
      ("pchild", po::value<double>()->default_value(0.5)->required(),
          "Param. for the geom. distr. for the number of children")
      ("pterm", po::value<double>()->default_value(0.5)->required(),
          "Param. for the geom. distr. for the number of target terminals")
      ("seed", po::value<unsigned int>()->default_value(0)->required(),
          "Seed for random generator")
      ("pcfg", "Use MLE PCFG estimates in the base distribution for trees")
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
  cerr << "Sampling with " << num_threads << " threads..." << endl;

  cerr << "Reading training data..." << endl;
  Dictionary dictionary;
  shared_ptr<vector<Instance>> training = make_shared<vector<Instance>>();
  ifstream tree_stream(vm["trees"].as<string>());
  ifstream string_stream(vm["strings"].as<string>());
  ifstream alignment_stream(vm["alignment"].as<string>());
  while (!tree_stream.eof() && !string_stream.eof() &&
         !alignment_stream.eof()) {
    training->push_back(ReadInstance(tree_stream, string_stream,
                                     alignment_stream, dictionary));
    tree_stream >> ws;
    string_stream >> ws;
    alignment_stream >> ws;
  }
  cerr << "Done..." << endl;

  shared_ptr<PCFGTable> pcfg_table;
  if (vm.count("pcfg")) {
    cerr << "Constructing PCFG table..." << endl;
    pcfg_table = make_shared<PCFGTable>(training);
    cerr << "Done..." << endl;
  }

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
      forward_stream, source_vocabulary, target_vocabulary, dictionary,
      num_threads);
  ifstream reverse_stream(vm["ibm1-reverse"].as<string>());
  reverse_table = make_shared<TranslationTable>(
      reverse_stream, target_vocabulary, source_vocabulary, dictionary,
      num_threads);
  cerr << "Done..." << endl;

  cerr << "Sampling..." << endl;
  unsigned int seed = vm["seed"].as<unsigned int>();
  if (seed == 0) {
    seed = time(NULL);
  }
  RandomGenerator generator(seed);
  Sampler sampler(training, dictionary, pcfg_table, forward_table,
                  reverse_table, generator, num_threads, vm.count("stats"),
                  vm.count("smart_expand"), vm["min_rule_count"].as<int>(),
                  vm.count("reorder"), vm["penalty"].as<double>(),
                  vm["max_leaves"].as<int>(), vm["max_tree_size"].as<int>(),
                  vm["alpha"].as<double>(), vm["pexpand"].as<double>(),
                  vm["pchild"].as<double>(), vm["pterm"].as<double>());
  string prefix = vm["output"].as<string>();
  sampler.Sample(prefix, vm["iterations"].as<int>(), vm["log_freq"].as<int>());
  cerr << "Done..." << endl;

  cerr << "Writing output files..." << endl;
  sampler.SerializeGrammar(prefix, vm.count("scfg"));
  if (vm.count("align")) {
    sampler.SerializeAlignments(prefix);
  }
  if (vm.count("reorder")) {
    sampler.SerializeReorderings(prefix);
  }
  cerr << "Done..." << endl;

  return 0;
}
