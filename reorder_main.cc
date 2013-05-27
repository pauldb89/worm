#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include "dictionary.h"
#include "viterbi_reorderer.h"

using namespace std;
namespace po = boost::program_options;

int main(int argc, char** argv) {
  po::options_description desc("Command line options");

  desc.add_options()
      ("help", "Show available options")
      ("grammar,g", po::value<string>()->required(), "Path to grammar file")
      ("alignment,a", po::value<string>()->required(),
          "Path to file containing rule alignments")
      ("penalty", po::value<double>()->default_value(0.1),
          "Displacement penalty for reordering");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("help")) {
    cout << desc << endl;
    return 0;
  }

  po::notify(vm);

  Dictionary dictionary;
  ifstream grammar_stream(vm["grammar"].as<string>());
  ifstream alignment_stream(vm["alignment"].as<string>());
  // TODO(pauldb): Avoid storing the grammar twice (here and in one of the
  // reordering models).
  Grammar grammar(grammar_stream, alignment_stream, dictionary,
                  vm["penalty"].as<double>());
  ViterbiReorderer reorderer(grammar);

  while (true) {
    cin >> ws;
    if (!cin.good()) {
      break;
    }

    AlignedTree tree = ReadParseTree(cin, dictionary);
    String reordering = reorderer.Reorder(tree);
    WriteTargetString(cout, reordering, dictionary);
    cout << "\n";
  }

  return 0;
}
