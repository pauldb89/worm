#include <vector>

#include "node.h"
#include "aligned_tree.h"

using namespace std;

typedef vector<pair<int, int>> Alignment;
typedef vector<StringNode> String;
typedef pair<AlignedTree, String> Instance;
typedef pair<AlignedTree, String> Rule;
typedef AlignedTree::iterator NodeIter;
