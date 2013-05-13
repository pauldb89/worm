#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/version.hpp>

#include "aligned_tree.h"
#include "dictionary.h"
#include "pcfg_table.h"
#include "sampler.h"
#include "translation_table.h"
#include "util.h"

using namespace std;
namespace po = boost::program_options;

int main(int argc, char **argv) {
  po::options_description desc("Command line options");
  desc.add_options()
      ("help,h", "Show available options")
      ("trees,t", po::value<string>()->required(),
          "File continaing source parse trees in .ptb format")
      ("strings,s", po::value<string>()->required(),
          "File continaing target strings")
      ("output,o", po::value<string>()->required(),
          "File for writing the grammar")
      ("alpha", po::value<double>()->default_value(1.0),
          "Dirichlet process concentration parameter")
      ("iterations", po::value<int>()->default_value(100),
          "Number of iterations")
      ("pexpand", po::value<double>()->default_value(0.5),
          "Param. for the Bernoulli distr. for a node to be split")
      ("pchild", po::value<double>()->default_value(0.5),
          "Param. for the geom. distr. for the number of children")
      ("pterm", po::value<double>()->default_value(0.5),
          "Param. for the geom. distr. for the number of target terminals")
      ("seed", po::value<unsigned int>()->default_value(0),
          "Seed for random generator")
      ("pcfg", "Use MLE PCFG estimates in the base distribution for trees")
      ("ibm1-source-vcb", po::value<string>(), "Giza++ source vocabulary file")
      ("ibm1-target-vcb", po::value<string>(), "Giza++ target vocabulary file")
      ("ibm1-forward", po::value<string>(),
          "Path to the IBM Model 1 translation table p(t|s). Expected format: "
          "target_word_id source_word_id probability.")
      ("ibm1-backward", po::value<string>(),
          "Path to the IBM Model 1 translation table p(s|t). Expected format: "
          "source_word_id target_word_id probability.");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("help")) {
    cout << desc << endl;
    return 0;
  }

  po::notify(vm);

  // Read training data.
  Dictionary dictionary;
  shared_ptr<vector<Instance>> training = make_shared<vector<Instance>>();
  ifstream tree_stream(vm["trees"].as<string>());
  ifstream string_stream(vm["strings"].as<string>());
  while (true) {
    tree_stream >> ws;
    string_stream >> ws;
    if (tree_stream.eof() || string_stream.eof()) {
      break;
    }
    training->push_back(ReadInstance(tree_stream, string_stream, dictionary));
  }

  shared_ptr<PCFGTable> pcfg_table;
  if (vm.count("pcfg")) {
    pcfg_table = make_shared<PCFGTable>(training);
  }

  shared_ptr<TranslationTable> forward_table, backward_table;
  if (vm.count("ibm1-forward") && vm.count("ibm1-backward") &&
      vm.count("ibm1-source-vcb") && vm.count("ibm1-target-vcb")) {
    ifstream source_vcb_stream(vm["ibm1-source-vcb"].as<string>());
    Dictionary source_vocabulary(source_vcb_stream);
    ifstream target_vcb_stream(vm["ibm1-target-vcb"].as<string>());
    Dictionary target_vocabulary(target_vcb_stream);

    ifstream forward_stream(vm["ibm1-forward"].as<string>());
    forward_table = make_shared<TranslationTable>(
        forward_stream, source_vocabulary, target_vocabulary, dictionary);
    ifstream backward_stream(vm["ibm1-backward"].as<string>());
    backward_table = make_shared<TranslationTable>(
        backward_stream, target_vocabulary, source_vocabulary, dictionary);
  }

  // Induce tree to string grammar via Gibbs sampling.
  unsigned int seed = vm["seed"].as<unsigned int>();
  if (seed == 0) {
    seed = time(NULL);
  }
  RandomGenerator generator(seed);
  Sampler sampler(training, dictionary, pcfg_table, forward_table,
                  backward_table, generator, vm["alpha"].as<double>(),
                  vm["pexpand"].as<double>(), vm["pchild"].as<double>(),
                  vm["pterm"].as<double>());
  sampler.Sample(vm["iterations"].as<int>());

  ofstream grammar_stream(vm["output"].as<string>());
  sampler.SerializeGrammar(grammar_stream);

  return 0;
}
