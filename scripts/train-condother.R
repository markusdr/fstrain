#!/usr/bin/env Rscript 

library("utils")
library("methods")
library("stats")

printHelp <- function() {
  cat("Trains a model.",
      "",
      "Available options:",
      "  --help",
      "  --fst",
      "  --align-fst",
      "  --train-data",
      "  --isymbols",
      "  --osymbols",
      "  --variance",
      "  --output",
      "  --random-range",
      "  --joint-training",
      "  --has-lenpenalties",
      "  --lenpenalty-step",
      "  --force-convergence",
      "  --verbose",
      "  --openfst-delta",
      "  --timelimit",
      "  --lbfgs-max-iterations",
      "  --lbfgs-convergence-factor",
      "  --init-weights",
      "  --ngram-order",
      "  --max-insertions",
      "  --num-conjugations",
      "  --num-change-regions",
      sep="\n")
}

myInclude <- function(scriptname) {
  LIBDIR=paste(Sys.getenv("FSTRAIN_HOME"), "scripts", sep=.Platform$file.sep)
  source(paste(LIBDIR, scriptname, sep=.Platform$file.sep))  
}

convergesForAnyInput <- function(params) {
  result = TRUE
  .C("FunctionConvergesForAnyInput", params, result=as.logical(result))$result
}

myInclude("parse-command-args.R")
myInclude("rprop2.R")

# Variable defaults
variance <- 10.0
joint.training <- FALSE
random.range <- 0.01
lenpenalty.step <- 0.5
force.convergence <- FALSE
verboseLevel <- 1
openFstDelta <- 1e-8
timelimit <- 3000
lbfgsMaxIterations <- 100
lbfgsConvergenceFactor <- 1e-12
ngramOrder <- 1
maxInsertions <- -1
numConjugations <- 0
numChangeRegions <- 0
rprop.on <- FALSE
rprop.tolerance <- 1e-3
lenmatch <- FALSE
lenmatch.variance <- 10.0
lenmatch.xy <- TRUE
check.gradients <- FALSE

programOptions = parseCommandArgs()

if(!is.null(programOptions$help)){
  printHelp()
  quit()
}

if(is.null(programOptions$fst)
   && is.null(programOptions$align.fst)
   && is.null(programOptions$ngram.order)) {
     write(paste("Please use one of the following:",
                 " (1) --fst,",
                 " (2) --align-fst=... --ngram-order=...",
                 " (3) --ngram-order=...",
                 sep="\n"),
           stderr())
  quit()
}

if(!is.null(programOptions$check.gradients)) {
  check.gradients <- TRUE
}

if(!is.null(programOptions$lenmatch)) {
  lenmatch <- TRUE
}
if(!is.null(programOptions$lenmatch.y)) {
  lenmatch.xy <- FALSE # just match y
}
if(!is.null(programOptions$lenmatch.variance)) {
  lenmatch.variance <- as.numeric(programOptions$lenmatch.variance)
}


if(!is.null(programOptions$rprop.on)) {
  rprop.on <- TRUE
}
if(!is.null(programOptions$rprop.tolerance)) {
  rprop.tolerance <- as.numeric(programOptions$rprop.tolerance)
}

if(is.null(programOptions$isymbols)) {
  print("Please use option --isymbols")
  quit()
}

if(is.null(programOptions$osymbols)) {
  print("Please use option --osymbols")
  quit()
}

if(is.null(programOptions$train.data)) {
  print("Please use option --train-data")
  quit()
}

if(!is.null(programOptions$other.list)) {
  other.list <- programOptions$other.list
}

if(!is.null(programOptions$other.other2in)) {
  other.other2in <- programOptions$other.other2in
}
if(!is.null(programOptions$other.other2out)) {
  other.other2out <- programOptions$other.other2out
}
if(!is.null(programOptions$other.syms)) {
  other.syms <- programOptions$other.syms
}
if(!is.null(programOptions$kbest)) {
  kbest <- programOptions$kbest
}

write("Options:", stdout())
print(programOptions);

if(!is.null(programOptions$variance)) {
  variance <- as.numeric(programOptions$variance)
}

if(!is.null(programOptions$random.range)) {
  random.range <- as.numeric(programOptions$random.range)
}

if(!is.null(programOptions$verbose)) {
  verboseLevel <- as.numeric(programOptions$verbose)
}

if(!is.null(programOptions$openfst.delta)) {
  openFstDelta <- as.numeric(programOptions$openfst.delta)
}

if(!is.null(programOptions$joint.training)) {
  joint.training <- TRUE
}

if(!is.null(programOptions$timelimit)) {
  timelimit <- as.numeric(programOptions$timelimit)
}

if(!is.null(programOptions$lbfgs.max.iterations)) {
  lbfgsMaxIterations <- as.integer(programOptions$lbfgs.max.iterations)
}

if(!is.null(programOptions$lbfgs.convergence.factor)) {
  lbfgsConvergenceFactor <- as.double(programOptions$lbfgs.convergence.factor)
}

if(!is.null(programOptions$force.convergence)) {
  force.convergence <- TRUE
}

if(!is.null(programOptions$ngram.order)) {
  ngramOrder <- as.integer(programOptions$ngram.order)
}

if(!is.null(programOptions$max.insertions)) {
  maxInsertions <- as.integer(programOptions$max.insertions)
}

if(!is.null(programOptions$num.conjugations)) {
  numConjugations <- as.integer(programOptions$num.conjugations)
}

if(!is.null(programOptions$num.change.regions)) {
  numChangeRegions <- as.integer(programOptions$num.change.regions)
}

buildDir=paste(Sys.getenv("FSTRAIN_HOME"), "build", sep=.Platform$file.sep)
if(!is.null(programOptions$build.dir)) {
  buildDir <- programOptions$build.dir;
}

DYNLIB.PATH = paste(buildDir, sep=.Platform$file.sep)
OBJECTIVE.FUNCTION.SO =
  paste(DYNLIB.PATH, .Platform$file.sep,
        "create+train-r-interface3",
        .Platform$dynlib.ext,
        sep="")
dyn.load(OBJECTIVE.FUNCTION.SO)

objectiveFunctionType = 3; # special length match objective,
                           # and condition on "other", third variable

setLengthVariance <- function(variance){.C("SetLengthVariance", variance)}
setMatchXY <- function(boolval){.C("SetMatchXY", boolval)}

if(!is.null(programOptions$fst)) {
  if(!is.null(programOptions$align.fst)) {
    write("Please do not use --fst and --align-fst together", stderr())
    quit(status=FALSE)
  }
  if(!is.null(programOptions$ngram.order)) {
    write("Please do not use --fst and --ngram-order together", stderr())
    quit(status=FALSE)
  }
  write(paste("# Model FST from", programOptions$fst),
        stderr())
  .C("Init_FromFile",
     as.integer(objectiveFunctionType),
     programOptions$fst,
     programOptions$train.data,
     programOptions$isymbols,
     programOptions$osymbols,
     as.double(variance));
} else if(!is.null(programOptions$align.fst)) {
  write(paste("# Model FST from aligning", programOptions$train.data,
              "using FST", programOptions$align.fst, ", then getting ",
              as.character(ngramOrder), "grams"),
        stderr())
  write("Calling Init_FromAlignment", stderr());
  .C("Init_FromAlignment",
     as.integer(objectiveFunctionType),
     as.integer(ngramOrder),  
     as.integer(maxInsertions),  
     as.integer(numConjugations),  
     as.integer(numChangeRegions),  
     programOptions$align.fst,
     programOptions$train.data,
     programOptions$isymbols,
     programOptions$osymbols,
     as.double(variance));
} else {
  write(paste("# Model FST: all", programOptions$ngram.order, " grams"),
        stderr())
  .C("Init_Ngram",
     as.integer(objectiveFunctionType),
     as.integer(ngramOrder),
     programOptions$train.data,
     programOptions$isymbols,
     programOptions$osymbols,
     as.double(variance));
}

#other.list <- "/home/hltcoe/mdreyer/morph/data/celex/german2/paradigms/0i-13SIA-3SIE-pA/sv2/train/input/pairs/13SIA-3SIE/train.25.0i";
#other.other2in <- "/home/hltcoe/mdreyer/fstrain-experiments/results/20091206-train/0i-13SIA-var1-lenvar1-rprop.fst"
#other.other2out <- "/home/hltcoe/mdreyer/fstrain-experiments/results/20091206-train/0i-3SIE-var1-lenvar1-rprop.fst"
#other.syms <- programOptions$isymbols
.C("SetOtherVariable", other.list, other.other2in, other.other2out, other.syms);
.C("SetKbest", as.integer(kbest));

numParams = 1
numParams <- .C("GetNumParameters", result=integer(1))$result
write(paste("Found", numParams, "params"), stderr())

if(!is.null(programOptions$init.weights)) {
  write(paste("Reading", programOptions$init.weights), stdout())
  initParams <- scan(programOptions$init.weights, sep="\n")
} else {
  # initParams = rnorm(numParams, mean=0.0, sd=random.range)
  initParams = runif(numParams, min=-random.range, max=random.range)
}

if(!is.null(programOptions$lenpenalty.step)) {
  lenpenalty.step = as.numeric(programOptions$lenpenalty.step)
}

if(force.convergence){
  if(!is.null(programOptions$has.lenpenalties)) {
    while(!convergesForAnyInput(initParams)) {
      write(paste("Adding", lenpenalty.step), stderr())
      initParams[1] <- initParams[1] + lenpenalty.step
      initParams[2] <- initParams[2] + lenpenalty.step
    }
  } else {
    while(!convergesForAnyInput(initParams)) {
      write(paste("Adding", lenpenalty.step, " (all params)"), stderr())
      initParams <- initParams + lenpenalty.step
    }
  }
}

.C("SetFstDelta", as.double(openFstDelta))
.C("SetTimelimit", as.double(timelimit))

obj <- function(x){.C("GetFunctionValue", x, result = double(1))$result}
gr <- function(x){.C("GetGradients", x, result = double(numParams))$result}

makeConverge <- function(x, lenpenalty.step){.C("MakeConvergent", result=x, lenpenalty.step)$result}
writeModel <- function(stem) { .C("Write", stem) }

if(lenmatch){
  setLengthVariance(lenmatch.variance)
  setMatchXY(lenmatch.xy)
}

if(check.gradients){
  write("CHECKING GRADIENTS", stderr())
  numcheck <- 100
  obj(initParams)
  # write(paste("PARAMS:", initParams), stderr())
  write(paste("GRADIENTS: ", gr(initParams)[1:min(numcheck, length(initParams))]), stderr())
  for(i in 1:(min(numcheck, length(initParams)))){
    eps <- 1e-6
    f1 <- obj(initParams)
    added <- rep(0.0, times=length(initParams))
    added[i] = -eps; 
    f2 <- obj(initParams+added)
    write(paste("Grad ", (i-1), ": ", (f2-f1)/eps), stderr())
  }
}

if(rprop.on) {
  write("# Optimization method: RPROP", stderr())
  #initParams <- .C("MakeConvergent", result=initParams, penaltyStepSize)$result
  result <- rprop(initParams, obj, gr, makeConverge, rprop.tolerance)
  #result <- list(par=initParams, value=100)

} else {
  # control<-list(trace=6, REPORT=1, maxit=500)
  control<-list(trace=verboseLevel, REPORT=1, fnscale=1e-2, maxit=lbfgsMaxIterations, factr=lbfgsConvergenceFactor)
  result <- optim(initParams, obj, gr, method="L-BFGS-B", control=control);
  # result <- optim(initParams, obj, NULL, method="L-BFGS-B", control=control);
  
  write(paste("Convergence: ", result$convergence), stderr())
  write(paste("Message: ", result$message), stderr())
}

if(!is.null(programOptions$output)){
  filename = paste(programOptions$output, ".feat-weights", sep="")
  write(paste("Writing", filename), stderr())
  write(result$par, file=filename, sep="\n")
  fstName = paste(programOptions$output, ".fst", sep="")
  .C("Save", fstName)
}

.C("Cleanup")
