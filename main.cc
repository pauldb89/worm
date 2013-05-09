#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/version.hpp>

#include "aligned_tree.h"
#include "dictionary.h"
#include "sampler.h"
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
          "Seed for random generator");

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
  ifstream tree_infile(vm["trees"].as<string>().c_str());
  ifstream string_infile(vm["strings"].as<string>().c_str());
  while (true) {
    tree_infile >> ws;
    string_infile >> ws;
    if (tree_infile.eof() || string_infile.eof()) {
      break;
    }

    training->push_back(ReadInstance(tree_infile, string_infile, dictionary));
  }

  // Induce tree to string grammar via Gibbs sampling.
  unsigned int seed = vm["seed"].as<unsigned int>();
  if (seed == 0) {
    seed = time(NULL);
  }
  RandomGenerator generator(seed);
  Sampler sampler(training, vm["alpha"].as<double>(),
                  vm["pexpand"].as<double>(), vm["pchild"].as<double>(),
                  vm["pterm"].as<double>(), generator, dictionary);
  sampler.Sample(vm["iterations"].as<int>());

  ofstream grammar_file(vm["output"].as<string>().c_str());
  sampler.SerializeGrammar(grammar_file);

  return 0;
}
