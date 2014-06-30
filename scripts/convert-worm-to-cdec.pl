#!/usr/bin/perl -w
use strict;

# LST ||| (LST (PU #0) (NP #1) (PU #2) ) ||| #0 #1 #2 ||| 0.00757576
while(<>) {
  chomp;
  my ($root, $src, $trg, $prob) = split / \|\|\| /;
  my $src_nts = $src =~ s/\(([^ ]+) #(\d+)\)/[$1]/g;
  $src =~ s/ +\)/)/g;
  my @trgs = split /\s+/, $trg;
  my @out = ();
  my %cov = ();
  my $trg_nts = 0;
  for my $t (@trgs) {
    if ($t =~ /^#(\d+)$/) {
      my $ind = $1 + 1;
      $cov{$ind}++;
      $trg_nts++;
      push @out, "[$ind]";
    } else {
      $t =~ s/\[/-LSB-/g;
      $t =~ s/\]/-RSB-/g;
      push @out, $t;
    }
  }
  unless ($src_nts) { $src_nts = 0; }
  if ($src_nts != $trg_nts) {
    warn "[ERROR] Mismatched numbers of source and target NTs in:\n$_\n";
  } else {
    for (my $i = 1; $i <= $trg_nts; $i++) {
      delete $cov{$i};
    }
    if (scalar keys %cov) {
      warn "[ERROR] Remaining NTs in:\n$_\n";
    } else {
      $prob = log($prob);
      print "$src ||| @out ||| LogProb=$prob\n";
    }
  }
}

