#ifndef _REORDERER_H_
#define _REORDERER_H_

#include <map>

#include "reorderer_base.h"

using namespace std;

typedef AlignedTree::iterator NodeIter;
typedef map<NodeIter, double> Cache;

class Reorderer : public ReordererBase {
 public:
  String Reorder(const AlignedTree& tree);

 private:
  Cache ConstructProbabilityCache(const AlignedTree& tree);

  String ConstructReordering(const Cache& cache, const AlignedTree& tree);
};

#endif
