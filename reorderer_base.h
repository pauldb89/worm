#ifndef _REORDERER_BASE_H_
#define _REORDERER_BASE_H_

#include "aligned_tree.h"
#include "util.h"

class ReordererBase {
 public:
  virtual String Reorder(const AlignedTree& tree) = 0;
};

#endif