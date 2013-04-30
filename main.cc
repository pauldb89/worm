#include <iostream>

#include <boost/program_options.hpp>

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

  return 0;
}
