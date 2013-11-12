#ifndef _PCFG_TABLE_H_
#define _PCFG_TABLE_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include <boost/functional/hash.hpp>

#include "util.h"

using namespace std;

typedef boost::hash<vector<int>> VectorHash;

class PCFGTable {
 public:
  PCFGTable(const shared_ptr<vector<Instance>>& training);

  double GetLogProbability(int lhs, const vector<int>& rhs) const;

 private:
  unordered_map<int, unordered_map<vector<int>, double, VectorHash>> rule_probs;
};

#endif
