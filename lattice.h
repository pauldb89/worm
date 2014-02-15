#pragma once

#include <string>
#include <tuple>
#include <vector>

#include "definitions.h"

using namespace std;

class Lattice {
 public:
  Lattice(const Distribution& distribution, Dictionary& dictionary);

  friend ostream& operator<<(ostream& out, const Lattice& lattice);

 private:
  vector<vector<tuple<string, double, int>>> graph;
};
