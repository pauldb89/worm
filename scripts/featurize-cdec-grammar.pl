#!/usr/bin/perl -w
use strict;

my %etot;
my %ftot;
my %ef;

while(<>) {
  chomp;
  my ($src, $trg, $pp) = split / \|\|\| /;
  my ($dd,$lp) = split /=/, $pp;
  my $p = exp($lp);
  # p = p(src, trg | root)
  # p(trg | root, src) = p(src,trg|root) / sum_{trg'} p(src,trg'|root)
  # denom = ftot
  $etot{$trg} += $p;
  $ftot{$src} += $p;
  $ef{"$src ||| $trg"} = $p;
}

for my $st (keys %ef) {
  my $p = $ef{$st};
  my $lp = log($p);
  my ($s, $t) = split / \|\|\| /, $st;
  my $tgs = $lp - log($ftot{$s});
  my $sgt = $lp - log($etot{$t});
  print "$s ||| $t ||| LogProb=$lp TGS=$tgs SGT=$sgt\n";
}

