#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using namespace std;
namespace fs = boost::filesystem;
namespace po = boost::program_options;

#include "aligned_tree.h"
#include "alignment_constructor.h"
#include "dictionary.h"
#include "translation_table.h"
#include "util.h"

int main(int argc, char** argv) {
  po::options_description cmdline_specific("Command line options");
  cmdline_specific.add_options()
      ("help,h", "Show available options")
      ("config,c", po::value<string>(), "Path to config file");

  po::options_description general_options;
  general_options.add_options()
      ("trees,t", po::value<string>()->required(),
          "File containing source parse trees in .ptb format")
      ("strings,s", po::value<string>()->required(),
          "File containing target strings")
      ("internal,i", po::value<string>()->required(),
          "File containing hidden alignment variables")
      ("output", po::value<string>()->required(), "Output directory")
      ("ibm1-source-vcb", po::value<string>()->required(),
          "Giza++ source vocabulary file")
      ("ibm1-target-vcb", po::value<string>()->required(),
          "Giza++ target vocabulary file")
      ("forward-prob", po::value<string>()->required(),
          "Path to the IBM Model 1 translation table p(t|s). Expected format: "
          "source_word_id target_word_id probability.")
      ("reverse-prob", po::value<string>()->required(),
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
  Dictionary dictionary;
  shared_ptr<TranslationTable> forward_table, reverse_table;
  LoadTranslationTables(vm, forward_table, reverse_table, dictionary);
  vector<Instance> training = LoadInternalState(vm, dictionary);

  string output_directory = vm["output"].as<string>();
  fs::path output_path(output_directory);
  if (!fs::exists(output_path)) {
    fs::create_directory(output_path);
  }
  if (output_directory.back() != '/') {
    output_directory = output_directory + "/";
  }

  ofstream fwd_out(GetOutputFilename(output_directory, "fwd_align"));
  ofstream rev_out(GetOutputFilename(output_directory, "rev_align"));
  AlignmentConstructor alignment_constructor(forward_table, reverse_table);
  for (const Instance& instance: training) {
    auto alignments = alignment_constructor.ExtractAlignments(instance);
    fwd_out << alignments.first << "\n";
    rev_out << alignments.second << "\n";
  }

  return 0;
}
