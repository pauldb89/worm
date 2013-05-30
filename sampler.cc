#include "sampler.h"

#include <chrono>

#include "node.h"
#include "pcfg_table.h"
#include "translation_table.h"

using namespace chrono;

typedef high_resolution_clock Clock;

Sampler::Sampler(const shared_ptr<vector<Instance>>& training,
                 Dictionary& dictionary,
                 const shared_ptr<PCFGTable>& pcfg_table,
                 const shared_ptr<TranslationTable>& forward_table,
                 const shared_ptr<TranslationTable>& reverse_table,
                 RandomGenerator& generator,
                 double alpha, double pexpand, double pchild, double pterm) :
    training(training),
    dictionary(dictionary),
    pcfg_table(pcfg_table),
    forward_table(forward_table),
    reverse_table(reverse_table),
    generator(generator),
    uniform_distribution(0, 1),
    alpha(alpha),
    prob_expand(log(pexpand)),
    prob_not_expand(log(1 - pexpand)),
    prob_stop_child(log(pchild)),
    prob_cont_child(log(1 - pchild)),
    prob_stop_str(log(pterm)),
    prob_cont_str(log(1 - pterm)) {
  for (auto instance: *training) {
    initial_order.push_back(make_shared<Instance>(instance));
  }

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

  cout << "Log-likelihood: " << fixed << ComputeDataLikelihood() << endl;
  for (int iter = 0; iter < iterations; ++iter) {
    Clock::time_point start_time = Clock::now();

    random_shuffle(training->begin(), training->end());
    for (auto& instance: *training) {
      CacheSentence(instance);
      SampleAlignments(instance);
      SampleSwaps(instance);
    }

    Clock::time_point stop_time = Clock::now();
    auto duration = duration_cast<milliseconds>(stop_time - start_time).count();
    cout << "Iteration " << iter << " completed in "
         << duration / 1000.0 << " seconds" << endl;
    cout << "Log-likelihood: " << fixed << ComputeDataLikelihood() << endl;
  }
}

void Sampler::InitializeRuleCounts() {
  for (auto instance: *training) {
    const AlignedTree& tree = instance.first;
    for (auto node = tree.begin(); node != tree.end(); ++node) {
      if (node->IsSplitNode()) {
        IncrementRuleCount(GetRule(instance, node));
      }
    }
  }
}

void Sampler::CacheSentence(const Instance& instance) {
  if (forward_table == nullptr || reverse_table == nullptr) {
    return;
  }

  vector<int> source_words;
  const AlignedTree& tree = instance.first;
  for (auto leaf = tree.begin_leaf(); leaf != tree.end_leaf(); ++leaf) {
    source_words.push_back(leaf->GetWord());
  }

  vector<int> target_words;
  for (auto node: instance.second) {
    target_words.push_back(node.GetWord());
  }

  forward_table->CacheSentence(source_words, target_words);
  reverse_table->CacheSentence(target_words, source_words);
}

double Sampler::ComputeDataLikelihood() {
  unordered_map<int, RuleCounts> new_counts;
  for (auto entry: counts) {
    new_counts[entry.first] = RuleCounts(alpha, false);
  }

  double likelihood = 0;
  for (auto instance: *training) {
    CacheSentence(instance);
    const AlignedTree& tree = instance.first;
    for (auto node = tree.begin(); node != tree.end(); ++node) {
      if (node->IsSplitNode()) {
        const Rule& rule = GetRule(instance, node);
        int root_tag = node->GetTag();
        double prob = ComputeLogBaseProbability(rule);
        likelihood += new_counts[root_tag].log_prob(rule, prob);
        new_counts[root_tag].increment(rule);
      }
    }
  }

  return likelihood;
}

void Sampler::SampleAlignments(const Instance& instance) {
  const AlignedTree& tree = instance.first;
  vector<NodeIter> schedule = GetRandomSchedule(tree);

  // For each node, sample a new alignment span.
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
    node->SetSpan(make_pair(-1, -1));
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

    bool sampled = false;
    for (size_t i = 1; i < probs.size(); ++i) {
      if (value <= probs[i]) {
        node->SetSpan(legal_spans[i - 1]);
        IncrementRuleCount(GetRule(instance, ancestor));
        IncrementRuleCount(GetRule(instance, node));
        sampled = true;
        break;
      }

      value = Log<double>::subtract(value, probs[i]);
    }

    assert(sampled);
  }
}

void Sampler::SampleSwaps(const Instance& instance) {
  const AlignedTree& tree = instance.first;
  set<NodeIter> frontier;

  for (auto node = tree.begin_post(); node != tree.end_post(); ++node) {
    // We can only swap descendants of split nodes.
    if (!node->IsSplitNode()) {
      continue;
    }

    vector<NodeIter> descendants = tree.GetSplitDescendants(node);
    // If there are no descendants, we have nothing to sample.
    if (descendants.empty()) {
      frontier.insert(node);
      continue;
    }

    // We only swap descendants located on the frontier because these nodes have
    // no descendants (and no additional constraints).
    descendants.erase(remove_if(descendants.begin(), descendants.end(),
        [&frontier](const NodeIter& node) -> bool {
          return frontier.count(node) == 0;
        }), descendants.end());

    random_shuffle(descendants.begin(), descendants.end());
    // Sample swaps for consecutive pairs of descendants.
    for (size_t i = 1; i < descendants.size(); i += 2) {
      const Rule& rule1 = GetRule(instance, node);
      const Rule& rule2 = GetRule(instance, descendants[i - 1]);
      const Rule& rule3 = GetRule(instance, descendants[i]);
      DecrementRuleCount(rule1);
      DecrementRuleCount(rule2);
      DecrementRuleCount(rule3);

      double prob_no_swap = ComputeLogProbability(rule1, rule2, rule3);

      // Swap spans.
      auto span1 = descendants[i - 1]->GetSpan();
      auto span2 = descendants[i]->GetSpan();
      descendants[i - 1]->SetSpan(span2);
      descendants[i]->SetSpan(span1);

      const Rule& srule1 = GetRule(instance, node);
      const Rule& srule2 = GetRule(instance, descendants[i - 1]);
      const Rule& srule3 = GetRule(instance, descendants[i]);

      double prob_swap = ComputeLogProbability(srule1, srule2, srule3);

      // Normalize probabilities.
      double total_prob = Log<double>::add(prob_no_swap, prob_swap);
      prob_swap -= total_prob;
      prob_no_swap -= total_prob;

      // Sample.
      double value = log(uniform_distribution(generator));
      if (value <= prob_no_swap) {
        // Replace with original spans.
        descendants[i - 1]->SetSpan(span1);
        descendants[i]->SetSpan(span2);

        IncrementRuleCount(rule1);
        IncrementRuleCount(rule2);
        IncrementRuleCount(rule3);
      } else {
        IncrementRuleCount(srule1);
        IncrementRuleCount(srule2);
        IncrementRuleCount(srule3);
      }
    }
  }
}

vector<NodeIter> Sampler::GetRandomSchedule(const AlignedTree& tree) {
  vector<NodeIter> schedule;
  for (NodeIter node = tree.begin(); node != tree.end(); ++node) {
    if (node != tree.begin()) {
      schedule.push_back(node);
    }
  }

  random_shuffle(schedule.begin(), schedule.end());
  return schedule;
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
    if (leaf != fragment.begin() && leaf->IsSplitNode()) {
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
      result.push_back(StringNode(-1, -1, frontier[i]));
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
      // See if the current nonterminal should count.
      if (pcfg_table == nullptr) {
        prob_frag += prob_nt;
      }

      // Check if the node expands or not.
      if (node->IsSplitNode()) {
        prob_frag += prob_not_expand;
        ++vars;
      } else {
        prob_frag += prob_expand;
      }
    }

    // Compute the probability associated with the rule that's currently
    // expanded.
    if (node == frag.begin() || !node->IsSplitNode()) {
      if (pcfg_table == nullptr) {
        if (node->IsSetWord()) {
          prob_frag += prob_st;
        } else {
          prob_frag += prob_cont_child * ((int) node.number_of_children() - 1);
          prob_frag += prob_stop_child;
        }
      } else {
        vector<int> rhs;
        if (node.number_of_children() > 0) {
          for (auto child = frag.begin(node); child != frag.end(node); ++child) {
            rhs.push_back(child->GetTag());
          }
        } else {
          rhs.push_back(node->GetWord());
        }

        prob_frag += pcfg_table->GetLogProbability(node->GetTag(), rhs);
      }
    }
  }

  double prob_str = 0.0;
  const String& target_string = rule.second;
  if (forward_table == nullptr || reverse_table == nullptr) {
    prob_str = prob_stop_str;
    prob_str += (prob_tt + prob_cont_str) * (target_string.size() - vars);
  } else {
    vector<int> source_indexes;
    for (auto leaf = frag.begin_leaf(); leaf != frag.end_leaf(); ++leaf) {
      if (leaf->IsSetWord() && (!leaf->IsSplitNode() || leaf == frag.begin())) {
        source_indexes.push_back(leaf->GetWordIndex());
      }
    }

    vector<int> target_indexes;
    for (auto node: target_string) {
      if (node.IsSetWord()) {
        target_indexes.push_back(node.GetWordIndex());
      }
    }

    // Use geometric mean on bidirectional IBM Model 1 probabilities.
    double prob_forward = forward_table->ComputeAverageLogProbability(
        source_indexes, target_indexes);
    double prob_reverse = reverse_table->ComputeAverageLogProbability(
        target_indexes, source_indexes);
    prob_str = 0.5 * (prob_forward + prob_reverse);
  }

  for (int i = 1; i <= vars; ++i) {
    prob_str -= log(target_string.size() - vars + i);
  }

  return prob_frag + prob_str;
}

double Sampler::ComputeLogProbability(const Rule& rule) {
  int tag = rule.first.GetRootTag();
  return counts[tag].log_prob(rule, ComputeLogBaseProbability(rule));
}

double Sampler::ComputeLogProbability(const Rule& r1, const Rule& r2) {
  double prob_r1 = ComputeLogProbability(r1);

  int tag = r2.first.GetRootTag();
  int same_rules = r1 == r2;
  int same_tags = r1.first.GetRootTag() == tag;
  return prob_r1 + counts[tag].log_prob(r2, same_rules, same_tags,
                                        ComputeLogBaseProbability(r2));
}

double Sampler::ComputeLogProbability(const Rule& r1, const Rule& r2,
                                      const Rule& r3) {
  double prob_r12 = ComputeLogProbability(r1, r2);

  int tag = r3.first.GetRootTag();
  int same_rules = (r1 == r3) + (r2 == r3);
  int same_tags = r1.first.GetRootTag() == tag || r2.first.GetRootTag() == tag;
  return prob_r12 + counts[tag].log_prob(r3, same_rules, same_tags,
                                         ComputeLogBaseProbability(r3));
}

void Sampler::IncrementRuleCount(const Rule& rule) {
  int tag = rule.first.GetRootTag();
  counts[tag].increment(rule);
}

void Sampler::DecrementRuleCount(const Rule& rule) {
  int tag = rule.first.GetRootTag();
  counts[tag].decrement(rule);
}

Alignment Sampler::ConstructNonterminalLinks(const Rule& rule) {
  const AlignedTree& frag = rule.first;
  const String& target_string = rule.second;

  Alignment alignment;
  int leaf_index = 0, var_index = 0;
  for (auto leaf = frag.begin_leaf(); leaf != frag.end_leaf(); ++leaf) {
    if (leaf->IsSplitNode() && leaf != frag.begin()) {
      for (size_t i = 0; i < target_string.size(); ++i) {
        if (target_string[i].GetVarIndex() == var_index) {
          alignment.push_back(make_pair(leaf_index, i));
          break;
        }
      }
      ++var_index;
    }
    ++leaf_index;
  }

  return alignment;
}

pair<Alignment, Alignment> Sampler::ConstructTerminalLinks(const Rule& rule) {
  const AlignedTree& frag = rule.first;
  const String& target_string = rule.second;

  Alignment forward_alignment;
  for (size_t i = 0; i < target_string.size(); ++i) {
    if (!target_string[i].IsSetWord()) {
      continue;
    }

    int target_word = target_string[i].GetWord(), leaf_index = 0;
    double best_match = forward_table->GetLogProbability(
        dictionary.NULL_WORD_ID, target_word);
    int best_index = -1;
    for (auto leaf = frag.begin_leaf(); leaf != frag.end_leaf(); ++leaf) {
      if (leaf->IsSetWord() && (!leaf->IsSplitNode() || leaf == frag.begin())) {
        double match_prob = forward_table->GetLogProbability(
            leaf->GetWord(), target_word);
        if (match_prob > best_match) {
          best_match = match_prob;
          best_index = leaf_index;
        }
      }

      ++leaf_index;
    }

    if (best_index >= 0) {
      forward_alignment.push_back(make_pair(best_index, i));
    }
  }

  Alignment reverse_alignment;
  int leaf_index = 0;
  for (auto leaf = frag.begin_leaf(); leaf != frag.end_leaf(); ++leaf) {
    if (leaf->IsSetWord() && (!leaf->IsSplitNode() || leaf == frag.begin())) {
      int source_word = leaf->GetWord();
      double best_match = reverse_table->GetLogProbability(
          dictionary.NULL_WORD_ID, source_word);
      int best_index = -1;
      for (size_t i = 0; i < target_string.size(); ++i) {
        if (!target_string[i].IsSetWord()) {
          continue;
        }

        double match_prob = reverse_table->GetLogProbability(
            target_string[i].GetWord(), source_word);
        if (match_prob > best_match) {
          best_match = match_prob;
          best_index = i;
        }
      }

      if (best_index >= 0) {
        reverse_alignment.push_back(make_pair(leaf_index, best_index));
      }
    }
    ++leaf_index;
  }

  return make_pair(forward_alignment, reverse_alignment);
}

pair<Alignment, Alignment> Sampler::ConstructAlignments(const Rule& rule) {
  Alignment forward_alignment, reverse_alignment;

  auto nonterminal_links = ConstructNonterminalLinks(rule);
  copy(nonterminal_links.begin(), nonterminal_links.end(),
       back_inserter(forward_alignment));
  copy(nonterminal_links.begin(), nonterminal_links.end(),
       back_inserter(reverse_alignment));

  auto terminal_links = ConstructTerminalLinks(rule);
  copy(terminal_links.first.begin(), terminal_links.first.end(),
       back_inserter(forward_alignment));
  copy(terminal_links.second.begin(), terminal_links.second.end(),
       back_inserter(reverse_alignment));

  return make_pair(forward_alignment, reverse_alignment);
}

void Sampler::SerializeAlignments(const string& output_prefix) {
  ofstream fwd_out(output_prefix + ".fwd_align");
  ofstream rev_out(output_prefix + ".rev_align");

  for (auto instance: initial_order) {
    const AlignedTree& tree = instance->first;

    Alignment forward_alignment, reverse_alignment;
    for (auto node = tree.begin(); node != tree.end(); ++node) {
      if (node->IsSplitNode()) {
        const Rule& rule = GetRule(*instance, node);
        const AlignedTree& frag = rule.first;
        const String& target_string = rule.second;

        auto subalignments = ConstructTerminalLinks(rule);

        vector<NodeIter> leaves;
        for (auto leaf = frag.begin_leaf(); leaf != frag.end_leaf(); ++leaf) {
          leaves.push_back(leaf);
        }

        for (auto link: subalignments.first) {
          int source_index = leaves[link.first]->GetWordIndex();
          int target_index = target_string[link.second].GetWordIndex();
          forward_alignment.push_back(make_pair(source_index, target_index));
        }

        for (auto link: subalignments.second) {
          int source_index = leaves[link.first]->GetWordIndex();
          int target_index = target_string[link.second].GetWordIndex();
          reverse_alignment.push_back(make_pair(source_index, target_index));
        }
      }
    }

    fwd_out << forward_alignment << "\n";
    rev_out << reverse_alignment << "\n";
  }
}

void Sampler::SerializeGrammar(const string& output_prefix, bool scfg_format) {
  unordered_map<int, map<Rule, double>> rule_probs;
  for (auto instance: *training) {
    CacheSentence(instance);
    const AlignedTree& tree = instance.first;
    for (NodeIter node = tree.begin(); node != tree.end(); ++node) {
      if (node->IsSplitNode()) {
        Rule rule = GetRule(instance, node);
        int root_tag = node->GetTag();
        if (rule_probs[root_tag].find(rule) == rule_probs[root_tag].end()) {
          rule_probs[root_tag][rule] = ComputeLogProbability(rule);
        }
      }
    }
  }

  ofstream gout(output_prefix + ".grammar");
  ofstream fwd_out(output_prefix + ".fwd");
  ofstream rev_out(output_prefix + ".rev");
  for (auto entry: rule_probs) {
    vector<pair<double, Rule>> rules;
    for (auto rule_entry: entry.second) {
      rules.push_back(make_pair(rule_entry.second, rule_entry.first));
    }

    sort(rules.begin(), rules.end(), greater<pair<double, Rule>>());
    for (auto rule: rules) {
      if (scfg_format) {
        WriteSCFGRule(gout, rule.second, dictionary);
      } else {
        WriteSTSGRule(gout, rule.second, dictionary);
      }
      gout << "||| " << exp(rule.first) << "\n";

      auto alignments = ConstructAlignments(rule.second);
      fwd_out << alignments.first << "\n";
      rev_out << alignments.second << "\n";
    }
  }
}
