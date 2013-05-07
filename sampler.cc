#include "sampler.h"

#include "node.h"

Sampler::Sampler(const shared_ptr<vector<Instance>>& training, double alpha,
                 double pexpand, double pchild, double pterm,
                 RandomGenerator& generator, Dictionary& dictionary) :
    training(training),
    prob_expand(log(pexpand)),
    prob_not_expand(log(1 - pexpand)),
    prob_stop_child(log(pchild)),
    prob_cont_child(log(1 - pchild)),
    prob_stop_str(log(pterm)),
    prob_cont_str(log(1 - pterm)),
    generator(generator),
    uniform_distribution(0, 1),
    dictionary(dictionary) {
  set<int> non_terminals, source_terminals, target_terminals;
  for (auto instance: *training) {
    for (auto node: instance.first) {
      if (!non_terminals.count(node.GetTag())) {
        non_terminals.insert(node.GetTag());
        counts[node.GetTag()] = RuleCounts(alpha, false);
      }

      if (node.IsSetWord()) {
        source_terminals.insert(node.GetWord());
      }
    }

    for (auto target_word: instance.second) {
      target_terminals.insert(target_word.GetWord());
    }
  }

  prob_nt = -log(non_terminals.size());
  prob_st = -log(source_terminals.size());
  prob_tt = -log(target_terminals.size());
}

void Sampler::Sample(int iterations) {
  InitializeRuleCounts();

  for (int iter = 0; iter < iterations; ++iter) {
    // Randomly shuffle the training set.
    random_shuffle(training->begin(), training->end());

    for (auto& instance: *training) {
      // Randomly shuffle nodes in the current parse tree.
      AlignedTree& tree = instance.first;
      vector<NodeIter> schedule;
      for (NodeIter node = tree.begin(); node != tree.end(); ++node) {
        if (node != tree.begin()) {
          schedule.push_back(node);
        }
      }

      random_shuffle(schedule.begin(), schedule.end());
      SampleAlignments(instance, schedule);
      SampleSwaps(schedule);
    }
  }
}

void Sampler::InitializeRuleCounts() {
  // Initially, the grammar consists of all the training instances.
  for (auto instance: *training) {
    IncrementRuleCount(instance);
  }
}

void Sampler::SampleAlignments(const Instance& instance,
                               const vector<NodeIter>& schedule) {
  const AlignedTree& tree = instance.first;
  for (auto node: schedule) {
    auto ancestor = tree.GetSplitAncestor(node);

    // Decrement existing rule counts.
    DecrementRuleCount(GetRule(instance, ancestor));
    if (node->IsSplitNode()) {
      DecrementRuleCount(GetRule(instance, node));
    }

    vector<double> probs;
    // Compute probability for not splitting the node (single rule).
    node->SetSplitNode(false);
    probs.push_back(ComputeLogProbability(GetRule(instance, ancestor)));

    // Find possible alignment spans and compute the probability for each one.
    node->SetSplitNode(true);
    auto legal_spans = GetLegalSpans(tree, node, ancestor);
    for (auto span: legal_spans) {
      node->SetSpan(span);
      const Rule& ancestor_rule = GetRule(instance, ancestor);
      const Rule& node_rule = GetRule(instance, node);
      probs.push_back(ComputeLogProbability(ancestor_rule, node_rule));
    }

    // Compute total probability
    double total_prob = Log<double>::zero();
    for (auto prob: probs) {
      total_prob = Log<double>::add(total_prob, prob);
    }

    // Normalize probabilities
    for (auto &prob: probs) {
      prob -= total_prob;
    }

    // Sample
    double value = log(uniform_distribution(generator));
    if (value <= probs[0]) {
      node->SetSplitNode(false);
      node->SetSpan(make_pair(-1, -1));
      IncrementRuleCount(GetRule(instance, ancestor));
      continue;
    } else {
      node->SetSplitNode(true);
      value = Log<double>::subtract(value, probs[0]);
    }

    for (size_t i = 1; i < probs.size(); ++i) {
      if (value <= probs[i]) {
        node->SetSpan(legal_spans[i - 1]);
        IncrementRuleCount(GetRule(instance, ancestor));
        IncrementRuleCount(GetRule(instance, node));
        break;
      }

      value = Log<double>::subtract(value, probs[i]);
    }
  }
}

void Sampler::SampleSwaps(const vector<NodeIter>& schedule) {
}

Rule Sampler::GetRule(const Instance& instance, const NodeIter& node) {
  AlignedTree fragment = instance.first.GetFragment(node);
  String target_string = ConstructRuleTargetSide(fragment, instance.second);
  return make_pair(fragment, target_string);
}

String Sampler::ConstructRuleTargetSide(const AlignedTree& fragment,
                                        const String& target_string) {
  pair<int, int> root_span = fragment.begin()->GetSpan();
  vector<int> frontier(root_span.second, -1);
  int num_split_leaves = 0;
  for (auto leaf = fragment.begin_leaf(); leaf != fragment.end_leaf(); ++leaf) {
    if (leaf != fragment.begin()) {
      pair<int, int> span = leaf->GetSpan();
      for (int j = span.first; j < span.second; ++j) {
        frontier[j] = num_split_leaves;
      }
      ++num_split_leaves;
    }
  }

  String result;
  for (int i = root_span.first; i < root_span.second; ++i) {
    if (frontier[i] == -1) {
      result.push_back(target_string[i]);
    } else if (result.empty() || result.back().GetVarIndex() != frontier[i]) {
      result.push_back(StringNode(-1, frontier[i]));
    }
  }

  return result;
}

vector<pair<int, int>> Sampler::GetLegalSpans(const AlignedTree& tree,
                                              const NodeIter& node,
                                              const NodeIter& ancestor) {
  pair<int, int> root_span = ancestor->GetSpan();
  vector<bool> include(root_span.second, false);
  vector<bool> exclude(root_span.second, false);

  // Exclude indexes contained by sibling nodes.
  vector<NodeIter> siblings = tree.GetSplitDescendants(ancestor);
  for (auto sibling: siblings) {
    if (sibling != node) {
      pair<int, int> span = sibling->GetSpan();
      for (int j = span.first; j < span.second; ++j) {
        exclude[j] = true;
      }
    }
  }

  // Include indexes contained by descendant nodes.
  int total_includes = 0;
  vector<NodeIter> descendants = tree.GetSplitDescendants(node);
  for (auto descendant: descendants) {
    pair<int, int> span = descendant->GetSpan();
    for (int j = span.first; j < span.second; ++j) {
      include[j] = true;
      ++total_includes;
    }
  }

  // Loop over all possible span candidates. A legal span is a span that
  // contains all includes and no excludes.
  vector<pair<int, int>> legal_spans;
  for (int start = root_span.first; start < root_span.second; ++start) {
    int includes = 0;
    for (int end = start + 1; end <= root_span.second; ++end) {
      if (exclude[end - 1]) {
        break;
      }

      includes += include[end - 1];
      if (includes == total_includes) {
        legal_spans.push_back(make_pair(start, end));
      }
    }
  }

  return legal_spans;
}

double Sampler::ComputeLogBaseProbability(const Rule& rule) {
  int vars = 0;
  double prob_frag = 0;
  const AlignedTree& frag = rule.first;
  for (NodeIter node = frag.begin(); node != frag.end(); ++node) {
    if (node != frag.begin()) {
      prob_frag += prob_nt;
      if (node->IsSplitNode()) {
        prob_frag += prob_expand;
        ++vars;
      } else {
        prob_frag += prob_not_expand;
      }
    }

    if (node == frag.begin() || !node->IsSplitNode()) {
      if (node->IsSetWord()) {
        prob_frag += prob_st;
      } else {
        prob_frag += prob_cont_child * ((int) node.number_of_children() - 1);
        prob_frag += prob_stop_child;
      }
    }
  }

  double prob_str = prob_stop_str;
  prob_str += (prob_tt + prob_cont_str) * (rule.second.size() - vars);
  for (int i = 1; i <= vars; ++i) {
    prob_str -= log(rule.second.size() - vars + i);
  }

  return prob_frag + prob_str;
}

double Sampler::ComputeLogProbability(const Rule& rule) {
  int tag = rule.first.begin()->GetTag();
  return counts[tag].log_prob(rule, ComputeLogBaseProbability(rule));
}

double Sampler::ComputeLogProbability(const Rule& r1, const Rule& r2) {
  double prob_r1 = ComputeLogProbability(r1);

  int tag = r2.first.begin()->GetTag();
  int same_rule = r1 == r2;
  int same_tag = r1.first.begin()->GetTag() == tag;
  return prob_r1 + counts[tag].log_prob(r2, same_rule, same_tag,
                                        ComputeLogBaseProbability(r2));
}

void Sampler::IncrementRuleCount(const Rule& rule) {
  int tag = rule.first.begin()->GetTag();
  counts[tag].increment(rule);
}

void Sampler::DecrementRuleCount(const Rule& rule) {
  int tag = rule.first.begin()->GetTag();
  counts[tag].decrement(rule);
}
