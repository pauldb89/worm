#!/usr/bin/perl -w
use strict;
my $lc = 0;
while(<>) {
  $lc++;
  my @terms = ();
  while(/ ([^()]+)\)/g) {
    my $term = $1;
    if ($term =~ /^#/) {
      die "Line $lc: tree contains # - this is a reserved word for the aligner.\n";
    } elsif ($term =~ /^(\[.+\])$/) {
      die "Line $lc: tree contains $1 - this is a reserved word for the aligner.\n";
    }
    push @terms, $term;
  }
  print "@terms\n";
}

