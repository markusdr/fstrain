#!/usr/bin/perl -w 

# Takes voc file (list of allowed words) and replaces all words in
# input that are not in voc file with UNK marker.

use Getopt::Long;

my $voc_file;

GetOptions(
	   "voc=s" => \$voc_file
	  ) or die("options error");
die("Please specify vocabulary file with --voc") unless defined $voc_file;

my %voc;
open(F, "<$voc_file") or die($!);
while(<F>){
  chomp;
  $voc{$_} = 1;
}
close F;

while(<>) {
  my $is_input_line = $. % 2 == 1;
  if($is_input_line){
    my @words = map {
      unless(exists $voc{$_}){
	s/.+\//UNK\//;
      }
      $_;
    } map {lcfirst $_} split(/\s+/, $_);
    if($words[0] eq "s"){
      $words[0] = "S";
    }
    if($words[-1] eq "e"){
      $words[-1] = "E";
    }
    print "@words\n";
  }
  else {
    print;
  }
}
