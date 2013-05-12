#!/usr/bin/perl -w 

# Prints list of high-frequency words
# Input example:
# S Confidence/NN in/IN the/DT pound/NN is/VBZ widely/RB expected/VBN to/TO take/VB another/DT sharp/JJ dive/NN if/IN trade/NN figures/NNS for/IN September/NNP ,/, due/JJ for/IN release/NN tomorrow/NN ,/, fail/VB to/TO show/VB a/DT substantial/JJ improvement/NN from/IN July/NNP and/CC August/NNP 's/POS near+record/JJ deficits/NNS ./. E
# S B+NP B+PP B+NP I+NP B+VP I+VP I+VP I+VP I+VP B+NP I+NP I+NP B+SBAR B+NP I+NP B+PP B+NP O B+ADJP B+PP B+NP B+NP O B+VP I+VP I+VP B+NP I+NP I+NP B+PP B+NP I+NP I+NP B+NP I+NP I+NP O E

use Getopt::Long;

my $min_freq = 2;

GetOptions(
	   "min-freq=i" => \$min_freq
	  ) or die("options error");

my %f;
while(<>) {
  my $is_input_line = $. % 2 == 1;
  if($is_input_line){
    my @words = map {lcfirst $_} split(/\s+/, $_);
    foreach(@words){
      ++$f{$_};
    }
  }
}

foreach(sort {$f{$b} <=> $f{$a}} keys %f) {
  if($f{$_} < $min_freq){
    last;
  }
  print "$_\n";
}
