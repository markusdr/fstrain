#!/usr/bin/perl -w 

# Extracts input and output symbol files from fstrain data file.
#
# Input example (input line always followed by output line):
# S Confidence/NN in/IN the/DT pound/NN E
# S B-NP B-PP B-NP I-NP E

use Getopt::Long;

my $outdir = ".";

GetOptions(
	   "out=s" => \$outdir
	  ) or die("options error");

my %isyms;
my %osyms;

my $isyms_file = "$outdir/isyms";
my $osyms_file = "$outdir/osyms";

while(<>) {
  my $is_input = $. % 2 == 1;
  if($is_input) {
    foreach(split /\s+/, $_) {
      $isyms{$_} = 1;
    }
  }
  else {
    foreach(split /\s+/, $_) {
      $osyms{$_} = 1;
    }
  }
}

WriteHash(\%isyms, $isyms_file);
WriteHash(\%osyms, $osyms_file);

sub WriteHash {
  my ($hashref, $filename) = @_;
  print STDERR "Writing $filename\n";
  open(F, ">$filename") or die($!);
  print F "eps 0\n";
  my $i = 0;    
  foreach my $key (sort keys %$hashref){
    print F "$key ", ++$i, "\n";
  }
  close F;
}
