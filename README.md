worm
====

A word reordering model that includes an implementation of the nonparametric Bayesian model for synchronous tree substitution grammar induction proposed by [Cohn and Blunsom (2009)](http://anthology.aclweb.org/D/D09/D09-1037.pdf).

### Get starting alignment and base parameters

TODO

### Infer grammars

    ./worm/sampler --alignment gdfa.al \
                   --trees fbis.parsed-zh \
                   --strings fbis.en \
                   --ibm1-forward fwd.probs \
                   --ibm1-reverse rev.probs \
                   --output worm-out \
                   --threads 6 \
                   --iterations 1000 &>log.sampler &

