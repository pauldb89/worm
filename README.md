worm
====

A word reordering model that includes an implementation of the nonparametric Bayesian model for synchronous tree substitution grammar induction proposed by [Cohn and Blunsom (2009)](http://anthology.aclweb.org/D/D09/D09-1037.pdf).

### Getting started

Prior to starting, you should
 * Build the sampler using [CMake](http://www.cmake.org/).
 * Install [cdec](http://www.cdec-decoder.org/), the root directory will be referred to as `$CDEC` in this document

### Preparing your parallel data

This section assumes you have a parallel corpus of source trees in `corpus.parsed-zh`, with in a one-tree-per-line format as follows:

    (IP (NP (NR 伊犁)) (VP (ADVP (AD 大规模)) (VP (VV 开展) (NP (IP (VP (VRD (PU “) (VV 面对面) (PU ”) (VV 宣讲)))) (NP (NN 活动))))))
    ...

And target strings in `corpus.en`, one sentence-per-line, tokenized (and, optionally, lowercased):

    yili launches large - scale ' face - to - face ' propaganda activity
    ...

You will need to extract the terminal sentences from `corpus.parsed-zh`, which can be done with the following command.

    ./worm/scripts/scripts/extract-terminals.pl corpus.parsed-zh > corpus.zh

You will now need to create training data `corpus.zh-en` for the word aligner that is used as the base distribution for the nonparametric tree aligner:

    $CDEC/corpus/paste-corpus.pl corpus.zh corpus.en > corpus.zh-en

### Initial word alignment and base parameters

In this section, a basic word alignment, `corpus.gdfa`, along with lexical translation probabilities are created.

    $CDEC/word-aligner/fast_align -i corpus.zh-en \
        -v -o -d -p fwd.probs -t -10000 > fwd.al
    $CDEC/word-aligner/fast_align -i corpus.zh-en \
        -v -o -d -p rev.probs -t -10000 -r > rev.al
    $CDEC/utils/atools -i fwd.probs -j rev.probs -c grow-diag-final-and > gdfa.al

### Infer grammars

This section describes running the TSG aligner. Outputs are written every 10 iterations to `worm-out/`.

    ./worm/sampler --alignment corpus.gdfa \
                   --trees corpus.parsed-zh \
                   --strings corpus.en \
                   --ibm1-forward fwd.probs \
                   --ibm1-reverse rev.probs \
                   --output worm-out \
                   --threads 6 \
                   --iterations 1000 &>log.sampler &

### Convert grammars to cdec format

Cdec supports [tree-to-string translation](http://www.cdec-decoder.org/concepts/xrs.html). To use the grammars produced by the sampler with cdec, they must be converted into the proper format, this can be done as follows:

    ./worm/scripts/convert-worm-to-cdec.pl worm-out/output.grammar |
       ./worm/scripts/featurize-cdec-grammar.pl | gzip -9 > rules.t2s.gz

