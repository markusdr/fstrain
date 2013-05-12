#!/usr/bin/perl -w 

use strict;
use Getopt::Long;

my $verbose = 1;
my $dryRun = 0;

my $trainData;
my $testData;
my $outstem;
my $variance = 1.0;
my $lenmatchVariance;
my $isymbols;
my $osymbols;
my $maxiter = 100;
my $rprop = 0;
my $rpropTolerance = 1e-3;
my $lenmatchY = 0; # match only y? default is x and y
my $ngramOrder = 3;
my $joint = 0;
my $staged = 1;
my $lenpenaltyFeatures = 1;
my $insdelFeatures = 0;
my $noepshistTopology = 0;
my $doTest = 1;
my $doPrune = 0;
my $pruneWeightThreshold = 0.0;
my $pruneStateFactor = 100;
my $timelimit = 500;
my $delta = 1e-6;
my $createFstFrom; # other data file used to get features and create FST from
my $eigenvalueMaxiter = 1000;
my $features;
my $backoff;
my $forceConvergence = 1;
my $randomRange = 0.01;
my $numThreads = 1;
my $maxInsertions = -1;
my $memoryLimitInGb = -1; # no limit
my $symThreshold = 1.0; # no arc sym pruning

GetOptions("train-data=s" => \$trainData,
	   "test-data=s" => \$testData,
	   "output=s" => \$outstem,
	   "isymbols=s" => \$isymbols,
	   "osymbols=s" => \$osymbols,
	   "variance=s" => \$variance,
	   "lenmatch-variance=s" => \$lenmatchVariance,
	   "lenmatch-y!" => \$lenmatchY,
	   "rprop!" => \$rprop, 
	   "ngram-order=i" => \$ngramOrder,
	   "rprop-tolerance=s" => \$rpropTolerance, 
	   "max-iterations=i" => \$maxiter,
	   "verbose!" => \$verbose,
	   "joint-training" => \$joint,
	   "features=s" => \$features,
	   "backoff=s" => \$backoff,
	   "lenpenalty-features!" => \$lenpenaltyFeatures,
	   "staged!" => \$staged,
	   "noepshist-topology" => \$noepshistTopology,
	   "do-prune!" => \$doPrune, 
	   "prune-weight-threshold=s" => \$pruneWeightThreshold,
	   "prune-state-factor=s" => \$pruneStateFactor,
	   "timelimit=i" => \$timelimit,
	   "delta=s" => \$delta,
 	   "dry-run!" => \$dryRun,
	   "create-fst-from=s" => \$createFstFrom,
	   "eigenvalue-maxiter=i" => \$eigenvalueMaxiter,
	   "force-convergence!" => \$forceConvergence,
	   "random-range=s" => \$randomRange,
	   "num-threads=i" => \$numThreads,
	   "max-insertions=i" => \$maxInsertions,
	   "memory-limit=s" => \$memoryLimitInGb,
	   "sym-threshold=s" => \$symThreshold,
	   "test!" => \$doTest
) or die("options error");

die("Please use --train-data and --test-data") 
    unless defined $trainData && defined $testData;
die("Please use --output") unless defined $outstem;
die("Please use --isymbols") unless defined $isymbols;
die("Please use --osymbols") unless defined $osymbols;

my $lenmatch = "";
if(defined $lenmatchVariance){
    $lenmatch = "--lenmatch --lenmatch.variance=$lenmatchVariance";
    if($lenmatchY){
	$lenmatch .= " --lenmatch.y";
    }
}

my $rpropOpt = "";
if($rprop){
    $rpropOpt = "--rprop.on --rprop.tolerance=$rpropTolerance";
}

my $jointOpt = "";
if($joint){
    $jointOpt = "--joint-training";
}

my $noepshistTopologyOpt = "";
if($noepshistTopology){
    $noepshistTopologyOpt = "--noepshist-topology";
}

my $lenpenaltyFeaturesOpt = $lenpenaltyFeatures 
  ? "--lenpenalty-features" 
  : "";
my $stagedOpt = $staged 
  ? "--init-weights=$outstem-align.feat-weights --init-names=$outstem-align.feat-names" 
  : "";

my $pruneOpt = $doPrune
  ? "--do-prune --prune-state-factor=$pruneStateFactor --prune-weight-threshold=$pruneWeightThreshold"
  : "";

my $featExtrOpt = "--features=";
if(!defined $features) {
  if($lenpenaltyFeatures) {
    $featExtrOpt .= "simple+len";
  }
  else {
    $featExtrOpt .= "simple";
  }
}
else {
  $featExtrOpt .= $features;
}

my $backoffOpt = "";
if(defined $backoff) {
  $backoffOpt = "--backoff=$backoff";
}

my $forceConvergenceOpt = $forceConvergence ? "--force-convergence" : "";

my $trainCmd1 = "./scripts/train.R --build.dir=./build --memory-limit=$memoryLimitInGb --max-insertions=$maxInsertions --num-threads=$numThreads --eigenvalue-maxiter=$eigenvalueMaxiter --openfst-delta=$delta --timelimit=$timelimit $lenmatch $rpropOpt $jointOpt --ngram-order=1 --isymbols=$isymbols --osymbols=$osymbols --train-data=$trainData --variance=$variance  $forceConvergenceOpt --random-range=$randomRange --output=$outstem-align --lbfgs-max-iterations=$maxiter >$outstem.train-align.stdout 2>$outstem.train-align.stderr";

my $trainCmd2;

if(defined $createFstFrom) {
  $trainCmd2 = "./scripts/train.R --build.dir=./build --sym-threshold=$symThreshold --memory-limit=$memoryLimitInGb --max-insertions=$maxInsertions --eigenvalue-maxiter=$eigenvalueMaxiter --openfst-delta=$delta --timelimit=$timelimit $lenmatch $rpropOpt $jointOpt --ngram-order=$ngramOrder --isymbols=$isymbols --osymbols=$osymbols --train-data=$createFstFrom --variance=$variance $forceConvergenceOpt --random-range=$randomRange --output=$outstem-init --lbfgs-max-iterations=0 --align-fst=$outstem-align.fst $pruneOpt $featExtrOpt $lenpenaltyFeaturesOpt $backoffOpt $stagedOpt $noepshistTopologyOpt >$outstem-init.train.stdout 2>$outstem-init.train.stderr; ";
  $trainCmd2 .= "./scripts/train.R --build.dir=./build --sym-threshold=$symThreshold --memory-limit=$memoryLimitInGb --max-insertions=$maxInsertions --num-threads=$numThreads --eigenvalue-maxiter=$eigenvalueMaxiter --openfst-delta=$delta --timelimit=$timelimit $lenmatch $rpropOpt $jointOpt --isymbols=$isymbols --osymbols=$osymbols --train-data=$trainData --variance=$variance --output=$outstem --lbfgs-max-iterations=$maxiter --fst=$outstem-init.fst $pruneOpt --init-weights=$outstem-init.feat-weights --init-names=$outstem-init.feat-names  $noepshistTopologyOpt >$outstem.train.stdout 2>$outstem.train.stderr";
}
else {
  $trainCmd2 = "./scripts/train.R --build.dir=./build --sym-threshold=$symThreshold --memory-limit=$memoryLimitInGb --max-insertions=$maxInsertions --num-threads=$numThreads --eigenvalue-maxiter=$eigenvalueMaxiter --openfst-delta=$delta --timelimit=$timelimit $lenmatch $rpropOpt $jointOpt --ngram-order=$ngramOrder --isymbols=$isymbols --osymbols=$osymbols --train-data=$trainData --variance=$variance $forceConvergenceOpt --random-range=$randomRange --output=$outstem --lbfgs-max-iterations=$maxiter --align-fst=$outstem-align.fst $pruneOpt $featExtrOpt $lenpenaltyFeaturesOpt $backoffOpt $stagedOpt $noepshistTopologyOpt >$outstem.train.stdout 2>$outstem.train.stderr";

}

my $testCmd = "./build/transducer-decode --isymbols=$isymbols --osymbols=$osymbols --fst=$outstem.fst --evaluate=true $testData >$outstem.test.stdout 2>$outstem.test.stderr";

safesystem($trainCmd1);
safesystem($trainCmd2);
if($doTest){
  safesystem($testCmd);
}

###

sub safesystem {
  print STDERR "\n# Executing: @_\n" if($verbose);
  unless($dryRun){
    system(@_) == 0 || die("Could not execute @_");
  }
}
