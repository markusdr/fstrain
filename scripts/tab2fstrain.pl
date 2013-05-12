#!/usr/bin/perl 

# Confidence NN B-NP
# in IN B-PP
# the DT B-NP

init();
while(<>) {
  chomp;
  my $sent_end = (length $_ == 0);
  if($sent_end){
    push @in, "E";
    push @out, "E";
    print "@in\n@out\n";
    init();
  }
  else {
    my @F = split /\s+/, $_;
    push @out, pop @F;
    push @in, join("/", @F);
  }
}

sub init {
  @in = ("S");
  @out = ("S");
}
