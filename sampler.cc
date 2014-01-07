#include "sampler.h"

#include <algorithm>

#include <omp.h>

#include "node.h"
#include "pcfg_table.h"
#include "time_util.h"
#include "translation_table.h"

Sampler::Sampler(const shared_ptr<vector<Instance>>& training,
                 Dictionary& dictionary,
                 const shared_ptr<PCFGTable>& pcfg_table,
                 const shared_ptr<TranslationTable>& forward_table,
                 const shared_ptr<TranslationTable>& reverse_table,
                 RandomGenerator& generator, int num_threads,
                 bool enable_all_stats, bool smart_expand,
                 int min_rule_count, bool reorder, double penalty,
                 int max_leaves, int max_tree_size, double alpha,
                 double pexpand, double pchild, double pterm,
                 const string& output_directory) :
    training(training),
    counts(num_threads, alpha),
    alignment_constructor(forward_table, reverse_table),
    dictionary(dictionary),
    pcfg_table(pcfg_table),
    forward_table(forward_table),
    reverse_table(reverse_table),
    generator(generator),
    uniform_distribution(0, 1),
    num_threads(num_threads),
    enable_all_stats(enable_all_stats),
    min_rule_count(min_rule_count),
    reorder(reorder),
    rule_reorderer(penalty, max_leaves, max_tree_size),
    reorder_counts(training->size()),
    smart_expand(smart_expand),
    alpha(alpha),
    prob_expand(log(pexpand)),
    prob_not_expand(log(1 - pexpand)),
    prob_stop_child(log(pchild)),
    prob_cont_child(log(1 - pchild)),
    prob_stop_str(log(pterm)),
    prob_cont_str(log(1 - pterm)),
    output_directory(output_directory) {
  set<int> non_terminals, source_terminals, target_terminals;
  // Do not parallelize.
  for (auto instance: *training) {
    for (auto node: instance.first) {
      if (!non_terminals.count(node.GetTag())) {
        non_terminals.insert(node.GetTag());
        counts.AddNonterminal(node.GetTag());
      }

      if (node.IsSetWord()) {
        source_terminals.insert(node.GetWord());
      }
    }

    for (auto target_word: instance.second) {
      target_terminals.insert(target_word.GetWord());
    }
  }

  if (smart_expand) {
    unordered_map<int, int> split_counts;
    for (auto instance: *training) {
      for (auto node: instance.first) {
        if (node.IsSplitNode()) {
          ++split_counts[node.GetTag()];
        }
      }
    }

    for (const auto& entry: split_counts) {
      double expand_prob = pow(entry.second, 1.8) + 1;
      expand_probs[entry.first] = -log(expand_prob);
      not_expand_probs[entry.first] = log((expand_prob - 1) / expand_prob);
    }
  }

  prob_nt = -log(non_terminals.size());
  prob_st = -log(source_terminals.size());
  prob_tt = -log(target_terminals.size());
}

void Sampler::Sample(int iterations, int log_frequency) {
  InitializeRuleCounts();

  counts.Synchronize();

  for (int iter = 0; iter < iterations; ++iter) {
    auto start_time = GetTime();
    DisplayStats();

    SerializeInternalState(to_string(iter));
    if (iter % log_frequency == 0) {
      cerr << "Serializing the grammar..." << endl;
      SerializeGrammar(false, to_string(iter));
      if (reorder) {
        SerializeReorderings(to_string(iter));
      }
      cerr << "Done..." << endl;
    }

    vector<int> schedule(training->size());
    iota(schedule.begin(), schedule.end(), 0);
    random_shuffle(schedule.begin(), schedule.end());
    #pragma omp parallel for schedule(dynamic) num_threads(num_threads)
    for (size_t i = 0; i < schedule.size(); ++i) {
      Instance& instance = (*training)[schedule[i]];

      // Ignore parse failures.
      if (instance.first.size() <= 1) {
        continue;
      }
      CacheSentence(instance);
      SampleAlignments(instance, schedule[i]);
      SampleSwaps(instance);
    }

    counts.Synchronize();

    if (reorder) {
      InferReorderings();
    }

    auto end_time = GetTime();
    cout << "Iteration " << iter << " completed in "
         << GetDuration(start_time, end_time) << " seconds" << endl;
  }
  DisplayStats();
}

void Sampler::InitializeRuleCounts() {
  #pragma omp parallel for schedule(dynamic) num_threads(num_threads)
  for (size_t i = 0; i < training->size(); ++i) {
    const Instance& instance = (*training)[i];
    const AlignedTree& tree = instance.first;
    for (auto node = tree.begin(); node != tree.end(); ++node) {
      if (node->IsSplitNode()) {
        IncrementRuleCount(extractor.ExtractRule(instance, node));
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

  int thread_id = omp_get_thread_num();
  forward_table->CacheSentence(source_words, target_words, thread_id);
  reverse_table->CacheSentence(target_words, source_words, thread_id);
}

void Sampler::DisplayStats() {
  auto start_time = GetTime();
  cout << "Log-likelihood: " << fixed << ComputeDataLikelihood() << endl;
  if (enable_all_stats) {
    cout << "\tAverage number of interior nodes: "
         << ComputeAverageNumInteriorNodes() << endl;
    cout << "\tGrammar size: " << GetGrammarSize() << endl;

    cout << "\tRule histogram: ";
    auto histogram = GenerateRuleHistogram();
    for (auto entry: histogram) {
      cout << "(" << entry.first << ", " << entry.second << ") ";
    }
    cout << endl;
  }
  auto end_time = GetTime();
  cerr << "Computing stats took: " << GetDuration(start_time, end_time)
       << " seconds..." << endl;
}

double Sampler::ComputeDataLikelihood() {
  DistributedRuleCounts new_counts(num_threads, alpha);
  for (auto nonterminal: counts.GetNonterminals()) {
    new_counts.AddNonterminal(nonterminal);
  }

  vector<double> likelihoods(num_threads);
  #pragma omp parallel for schedule(dynamic) num_threads(num_threads)
  for (size_t i = 0; i < training->size(); ++i) {
    int thread_id = omp_get_thread_num();
    const Instance& instance = (*training)[i];
    CacheSentence(instance);
    const AlignedTree& tree = instance.first;
    for (auto node = tree.begin(); node != tree.end(); ++node) {
      if (node->IsSplitNode()) {
        const Rule& rule = extractor.ExtractRule(instance, node);
        double prob = ComputeLogBaseProbability(rule);
        likelihoods[thread_id] += new_counts.GetLogProbability(rule, prob);
        new_counts.Increment(rule);
      }
    }
  }

  double likelihood = 0;
  for (int i = 0; i < num_threads; ++i) {
    likelihood += likelihoods[i];
  }

  return likelihood;
}

double Sampler::ComputeAverageNumInteriorNodes() {
  vector<int> interior_nodes(num_threads), total_rules(num_threads);
  #pragma omp parallel for schedule(dynamic) num_threads(num_threads)
  for (size_t i = 0; i < training->size(); ++i) {
    int thread_id = omp_get_thread_num();
    const AlignedTree& tree = (*training)[i].first;
    for (auto node = tree.begin(); node != tree.end(); ++node) {
      if (!node->IsSplitNode()) {
        ++interior_nodes[thread_id];
      } else {
        ++total_rules[thread_id];
      }
    }
  }

  int interior_nodes_sum = 0, total_rules_sum = 0;
  for (int i = 0; i < num_threads; ++i) {
    interior_nodes_sum += interior_nodes[i];
    total_rules_sum += total_rules[i];
  }

  cerr << "\tTotal rules: " << total_rules_sum << endl;
  return (double) interior_nodes_sum / total_rules_sum;
}

int Sampler::GetGrammarSize() {
  vector<set<Rule>> grammars(num_threads);
  #pragma omp parallel for schedule(dynamic) num_threads(num_threads)
  for (size_t i = 0; i < training->size(); ++i) {
    int thread_id = omp_get_thread_num();
    const Instance& instance = (*training)[i];
    const AlignedTree& tree = instance.first;
    for (auto node = tree.begin(); node != tree.end(); ++node) {
      if (node->IsSplitNode()) {
        grammars[thread_id].insert(extractor.ExtractRule(instance, node));
      }
    }
  }

  set<Rule> grammar;
  for (int i = 0; i < num_threads; ++i) {
    grammar.insert(grammars[i].begin(), grammars[i].end());
  }
  return grammar.size();
}

map<int, int> Sampler::GenerateRuleHistogram() {
  vector<map<int, int>> histograms(num_threads);
  #pragma omp parallel for schedule(dynamic) num_threads(num_threads)
  for (size_t i = 0; i < training->size(); ++i) {
    int thread_id = omp_get_thread_num();
    const AlignedTree& tree = (*training)[i].first;
    for (auto node = tree.begin(); node != tree.end(); ++node) {
      if (node->IsSplitNode()) {
        const AlignedTree& frag = tree.GetFragment(node);
        int inner_nodes = frag.size() - 1;

        if (frag.size() > 1) {
          for (auto leaf = frag.begin_leaf(); leaf != frag.end_leaf(); ++leaf) {
            if (leaf->IsSplitNode()) {
              --inner_nodes;
            }
          }
        }

        ++histograms[thread_id][inner_nodes];
      }
    }
  }

  map<int, int> histogram;
  for (int i = 0; i < num_threads; ++i) {
    for (const auto& entry: histograms[i]) {
      histogram[entry.first] += entry.second;
    }
  }

  return histogram;
}

void Sampler::SampleAlignments(const Instance& instance, int index) {
  const AlignedTree& tree = instance.first;
  vector<NodeIter> schedule = GetRandomSchedule(tree);

  // For each node, sample a new alignment span.
  for (auto node: schedule) {
    auto ancestor = tree.GetSplitAncestor(node);

    // Decrement existing rule counts.
    DecrementRuleCount(extractor.ExtractRule(instance, ancestor));
    if (node->IsSplitNode()) {
      DecrementRuleCount(extractor.ExtractRule(instance, node));
    }

    vector<double> probs;
    // Compute probability for not splitting the node (single rule).
    node->SetSplitNode(false);
    node->SetSpan(make_pair(-1, -1));
    const Rule& monolithic_rule = extractor.ExtractRule(instance, ancestor);
    probs.push_back(ComputeLogProbability(monolithic_rule));

    // Find possible alignment spans and compute the probability for each one.
    node->SetSplitNode(true);
    auto legal_spans = GetLegalSpans(tree, node, ancestor);
    for (auto span: legal_spans) {
      node->SetSpan(span);
      const Rule& ancestor_rule = extractor.ExtractRule(instance, ancestor);
      const Rule& node_rule = extractor.ExtractRule(instance, node);
      probs.push_back(ComputeLogProbability(ancestor_rule, node_rule));
    }

    // Compute total probability
    double total_prob = Log<double>::zero();
    for (auto prob: probs) {
      total_prob = Log<double>::add(total_prob, prob);
    }

    // TODO(pauldb): Maybe log-multiply (add) total_prob to value instead?
    // Normalize probabilities
    for (auto &prob: probs) {
      prob -= total_prob;
    }

    // Sample
    double value = log(uniform_distribution(generator));
    if (value <= probs[0]) {
      node->SetSplitNode(false);
      node->SetSpan(make_pair(-1, -1));
      IncrementRuleCount(extractor.ExtractRule(instance, ancestor));
      continue;
    } else {
      node->SetSplitNode(true);
      value = Log<double>::subtract(value, probs[0]);
    }

    bool sampled = false;
    for (size_t i = 1; i < probs.size(); ++i) {
      if (value <= probs[i]) {
        node->SetSpan(legal_spans[i - 1]);
        IncrementRuleCount(extractor.ExtractRule(instance, ancestor));
        IncrementRuleCount(extractor.ExtractRule(instance, node));
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
      const Rule& rule1 = extractor.ExtractRule(instance, node);
      const Rule& rule2 = extractor.ExtractRule(instance, descendants[i - 1]);
      const Rule& rule3 = extractor.ExtractRule(instance, descendants[i]);
      DecrementRuleCount(rule1);
      DecrementRuleCount(rule2);
      DecrementRuleCount(rule3);

      double prob_no_swap = ComputeLogProbability(rule1, rule2, rule3);

      // Swap spans.
      auto span1 = descendants[i - 1]->GetSpan();
      auto span2 = descendants[i]->GetSpan();
      descendants[i - 1]->SetSpan(span2);
      descendants[i]->SetSpan(span1);

      const Rule& srule1 = extractor.ExtractRule(instance, node);
      const Rule& srule2 = extractor.ExtractRule(instance, descendants[i - 1]);
      const Rule& srule3 = extractor.ExtractRule(instance, descendants[i]);

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
      int tag = node->GetTag();
      assert(!smart_expand || not_expand_probs.count(tag));
      assert(!smart_expand || expand_probs.count(tag));
      if (node->IsSplitNode()) {
        prob_frag += smart_expand ? not_expand_probs[tag] : prob_not_expand;
        ++vars;
      } else {
        prob_frag += smart_expand ? expand_probs[tag] : prob_expand;
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
        source_indexes, target_indexes, omp_get_thread_num());
    double prob_reverse = reverse_table->ComputeAverageLogProbability(
        target_indexes, source_indexes, omp_get_thread_num());
    prob_str = 0.5 * (prob_forward + prob_reverse);
  }

  for (int i = 1; i <= vars; ++i) {
    prob_str -= log(target_string.size() - vars + i);
  }

  return prob_frag + prob_str;
}

double Sampler::ComputeLogProbability(const Rule& rule) {
  return counts.GetLogProbability(rule, ComputeLogBaseProbability(rule));
}

double Sampler::ComputeLogProbability(const Rule& r1, const Rule& r2) {
  double prob_r1 = ComputeLogProbability(r1);

  int same_rules = r1 == r2;
  int same_tags = r1.first.GetRootTag() == r2.first.GetRootTag();
  return prob_r1 + counts.GetLogProbability(
      r2, same_rules, same_tags, ComputeLogBaseProbability(r2));
}

double Sampler::ComputeLogProbability(const Rule& r1, const Rule& r2,
                                      const Rule& r3) {
  double prob_r12 = ComputeLogProbability(r1, r2);

  int same_rules = (r1 == r3) + (r2 == r3);
  int same_tags = (r1.first.GetRootTag() == r3.first.GetRootTag()) +
                  (r2.first.GetRootTag() == r3.first.GetRootTag());
  return prob_r12 + counts.GetLogProbability(
      r3, same_rules, same_tags, ComputeLogBaseProbability(r3));
}

void Sampler::IncrementRuleCount(const Rule& rule) {
  counts.Increment(rule);
}

void Sampler::DecrementRuleCount(const Rule& rule) {
  counts.Decrement(rule);
}

void Sampler::InferReorderings() {
  cerr << "Inferring reorderings..." << endl;
  #pragma omp parallel for schedule(dynamic) num_threads(num_threads)
  for (size_t i = 0; i < training->size(); ++i) {
    const Instance& instance = (*training)[i];
    const AlignedTree& tree = instance.first;

    // Ignore parse failures.
    if (tree.size() <= 1) {
      continue;
    }

    CacheSentence(instance);
    String reordering;
    ExtractReordering(instance, tree.begin(), reordering);
    ++reorder_counts[i][reordering];
  }
  cerr << "Done..." << endl;
}

void Sampler::SerializeAlignments(const string& iteration) {
  ofstream fwd_out(GetOutputFilename(output_directory, iteration, "fwd_align"));
  ofstream rev_out(GetOutputFilename(output_directory, iteration, "rev_align"));
  // Do not parallelize.
  for (auto instance: *training) {
    const AlignedTree& tree = instance.first;

    if (tree.size() <= 1) {
      fwd_out << "\n";
      rev_out << "\n";
      continue;
    }

    Alignment forward_alignment, reverse_alignment;
    for (auto node = tree.begin(); node != tree.end(); ++node) {
      if (node->IsSplitNode()) {
        const Rule& rule = extractor.ExtractRule(instance, node);
        const AlignedTree& frag = rule.first;
        const String& target_string = rule.second;

        auto subalignments = alignment_constructor.ConstructTerminalLinks(rule);

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

void Sampler::SerializeGrammar(bool scfg_format, const string& iteration) {
  vector<unordered_map<int, map<Rule, int>>> rule_counts(num_threads);
  vector<unordered_map<int, map<Rule, double>>> rule_probs(num_threads);
  #pragma omp parallel for schedule(dynamic) num_threads(num_threads)
  for (size_t i = 0; i < training->size(); ++i) {
    int thread_id = omp_get_thread_num();
    const Instance& instance = (*training)[i];
    const AlignedTree& tree = instance.first;

    // Ignore parse failures.
    if (tree.size() <= 1) {
      continue;
    }

    CacheSentence(instance);
    for (NodeIter node = tree.begin(); node != tree.end(); ++node) {
      if (node->IsSplitNode()) {
        Rule rule = extractor.ExtractRule(instance, node);
        ++rule_counts[thread_id][node->GetTag()][rule];
        rule_probs[thread_id][node->GetTag()][rule] = exp(ComputeLogProbability(rule));
      }
    }
  }

  unordered_map<int, map<Rule, int>> aggregate_counts;
  unordered_map<int, map<Rule, double>> aggregate_probs;
  for (int i = 0; i < num_threads; ++i) {
    for (const auto& tag_entry: rule_counts[i]) {
      for (const auto& entry: tag_entry.second) {
        aggregate_counts[tag_entry.first][entry.first] += entry.second;
      }
    }
    for (const auto& tag_entry: rule_probs[i]) {
      for (const auto& entry: tag_entry.second) {
        aggregate_probs[tag_entry.first][entry.first] = entry.second;
      }
    }
  }

  ofstream gout(GetOutputFilename(output_directory, iteration, "grammar"));
  ofstream fwd_out(GetOutputFilename(output_directory, iteration, "fwd"));
  ofstream rev_out(GetOutputFilename(output_directory, iteration, "rev"));
  for (const auto& entry: aggregate_counts) {
    double total_rule_count = 0;
    for (const auto& rule_entry: entry.second) {
      if (rule_entry.second >= min_rule_count) {
        total_rule_count += rule_entry.second;
      }
    }

    vector<pair<double, Rule>> rules;
    for (const auto& rule_entry: entry.second) {
      if (rule_entry.second >= min_rule_count) {
        double rule_prob = 0;
        if (min_rule_count == 0) {
          rule_prob = aggregate_probs[entry.first][rule_entry.first];
        } else {
          rule_prob = rule_entry.second / total_rule_count;
        }
        rules.push_back(make_pair(rule_prob, rule_entry.first));
      }
    }

    sort(rules.begin(), rules.end(), greater<pair<double, Rule>>());
    for (auto rule: rules) {
      if (scfg_format) {
        WriteSCFGRule(gout, rule.second, dictionary);
      } else {
        WriteSTSGRule(gout, rule.second, dictionary);
      }
      gout << "||| " << rule.first << "\n";

      auto alignments = alignment_constructor.ConstructAlignments(rule.second);
      fwd_out << alignments.first << "\n";
      rev_out << alignments.second << "\n";
    }
  }
}

void Sampler::ExtractReordering(
    const Instance& instance, const NodeIter& node, String& reordering) {
  const Rule& rule = extractor.ExtractRule(instance, node);
  const auto& alignment = alignment_constructor.ConstructAlignments(rule).first;

  const auto& frontier = instance.first.GetSplitDescendants(node);
  const auto& reorder_frontier = rule_reorderer.Reorder(rule.first, alignment);
  for (const auto& descendant: reorder_frontier) {
    if (descendant.IsSetWord()) {
      reordering.push_back(descendant);
    } else {
      ExtractReordering(instance, frontier[descendant.GetVarIndex()],
                        reordering);
    }
  }
}

void Sampler::SerializeReorderings(const string& iteration) {
  ofstream out(GetOutputFilename(output_directory, iteration, "reorder"));
  ofstream dump_out(GetOutputFilename(output_directory, iteration, "dump"));

  for (const auto& counts: reorder_counts) {
    dump_out << counts.size() << "\n";

    int max_counts = 0;
    String best_reordering;
    for (const auto& entry: counts) {
      WriteTargetString(dump_out, entry.first, dictionary);
      dump_out << "\n" << entry.second << "\n";

      if (entry.second > max_counts) {
        best_reordering = entry.first;
        max_counts = entry.second;
      }
    }

    WriteTargetString(out, best_reordering, dictionary);
    out << "\n";
  }
}

void Sampler::SerializeInternalState(const string& iteration) {
  cerr << "Serializing internal state..." << endl;
  auto start_time = GetTime();

  ofstream out(GetOutputFilename(output_directory, iteration, "internal"));
  for (size_t i = 0; i < training->size(); ++i) {
    out << "####### Tree: " << i << " #######" << "\n";
    for (auto node: (*training)[i].first) {
      auto span = node.GetSpan();
      out << dictionary.GetToken(node.GetTag()) << " " << span.first << " "
          << span.second << "\n";
    }
  }

  auto end_time = GetTime();
  cerr << "Internal state serialized in " << GetDuration(start_time, end_time)
       << " seconds..." << endl;
}
