#include <chrono>
#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include "dictionary.h"
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
      ("penalty", po::value<double>()->default_value(0.1),
          "Displacement penalty for reordering");

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
  // TODO(pauldb): Avoid storing the grammar twice (here and in one of the
  // reordering models).
  Grammar grammar(grammar_stream, alignment_stream, dictionary,
                  vm["penalty"].as<double>());
  cerr << "Done..." << endl;

  ViterbiReorderer reorderer(grammar);

  int sentence_index = 0;
  Clock::time_point start_time = Clock::now();
  while (true) {
    cin >> ws;
    if (!cin.good()) {
      break;
    }

    AlignedTree tree = ReadParseTree(cin, dictionary);
    String reordering = reorderer.Reorder(tree, dictionary);
    if (reordering.size() == 0) {
      cerr << "Failed to reorder sentence " << sentence_index << ": ";
      tree.Write(cerr, dictionary);
      cerr << "\n";

      int word_index = 0;
      String source_string;
      for (auto leaf = tree.begin_leaf(); leaf != tree.end_leaf(); ++leaf) {
        source_string.push_back(StringNode(leaf->GetWord(), word_index++, -1));
      }
      WriteTargetString(cout, source_string, dictionary);
      cout << "\n";
    } else {
      WriteTargetString(cout, reordering, dictionary);
      cout << "\n";
    }

    ++sentence_index;
    if (sentence_index % 10 == 0) {
      Clock::time_point stop_time = Clock::now();
      auto diff = duration_cast<milliseconds>(stop_time - start_time).count();
      cerr << "Reordered " << sentence_index << " sentences in "
           << diff / 1000.0 << " seconds..." << endl;
    }
  }

  return 0;
}
