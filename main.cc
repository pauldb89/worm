#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

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
          "File continaing target strings");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("help")) {
    cout << desc << endl;
    return 0;
  }

  po::notify(vm);

  Dictionary dictionary;
  vector<Instance> training;
  ifstream tree_infile(vm["trees"].as<string>().c_str());
  ifstream string_infile(vm["strings"].as<string>().c_str());
  while (true) {
    tree_infile >> ws;
    string_infile >> ws;
    if (tree_infile.eof() || string_infile.eof()) {
      break;
    }

    training.push_back(ReadInstance(tree_infile, string_infile, dictionary));
  }

  return 0;
}
