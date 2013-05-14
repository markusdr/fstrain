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
      "  --lenpenalty-features",
      "  --lenpenalty-step",
      "  --num-threads",
      "  --force-convergence",
      "  --verbose",
      "  --openfst-delta",
      "  --timelimit",
      "  --lbfgs-max-iterations",
      "  --lbfgs-convergence-factor",
      "  --init-weights",
      "  --init-weights-perturb",
      "  --init-names",
      "  --ngram-order",
      "  --max-insertions",
      "  --num-conjugations",
      "  --num-change-regions",
      "  --do-prune",
      "  --prune-weight-threshold",
      "  --prune-state-factor",
      "  --backoff",
      "  --matrix-distance",
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
random.positive <- FALSE
lenpenalty.step <- 0.5
force.convergence <- FALSE
verboseLevel <- 1
openFstDelta <- 1e-8
timelimit <- 500
lbfgsMaxIterations <- 100
lbfgsConvergenceFactor <- 1e7
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
init.weights <- NULL
init.weights.perturb <- 0.0
init.names <- ""
features <- ""
doPrune <- FALSE
pruneWeightThreshold <- 0.0
pruneStateFactor <- 100
backoff <- ""
numThreads <- 1

write("------", stderr())
system("hostname")
system("date")
write(paste("pid: ", Sys.getpid()), stderr())
write("------\n", stderr())

# if TRUE, then constructed FSTs will have length penalty features as
# features 0 and 1, and an FST passed in with --fst must have it too:
lenpenalty.features <- FALSE

programOptions <- parseCommandArgs()

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

if(!is.null(programOptions$init.weights)) {
  init.weights <- programOptions$init.weights
}

if(!is.null(programOptions$init.weights.perturb)) {
 init.weights.perturb <- as.numeric(programOptions$init.weights.perturb)
}

if(!is.null(programOptions$init.names)) {
  init.names <- programOptions$init.names
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
  rprop.tolerance <- programOptions$rprop.tolerance
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

# e.g. --backoff=tlm,vc,id
if(!is.null(programOptions$backoff)) {
  backoff <- programOptions$backoff
}

write("Options:", stdout())
print(programOptions);

if(!is.null(programOptions$variance)) {
  variance <- as.numeric(programOptions$variance)
}

if(!is.null(programOptions$random.range)) {
  random.range <- as.numeric(programOptions$random.range)
}

if(!is.null(programOptions$random.positive)) {
  write("Initializing new features to positive values", stderr())
  random.positive <- TRUE
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

if(!is.null(programOptions$do.prune)) {
  doPrune <- TRUE
}

if(!is.null(programOptions$prune.weight.threshold)) {
  pruneWeightThreshold <- as.double(programOptions$prune.weight.threshold)
}

if(!is.null(programOptions$prune.state.factor)) {
  pruneStateFactor <- as.double(programOptions$prune.state.factor)
}

buildDir<-paste(Sys.getenv("FSTRAIN_HOME"), "build", sep=.Platform$file.sep)
if(!is.null(programOptions$build.dir)) {
  buildDir <- programOptions$build.dir;
}

DYNLIB.PATH <- paste(buildDir, sep=.Platform$file.sep)
OBJECTIVE.FUNCTION.SO <-
  paste(DYNLIB.PATH, .Platform$file.sep,
        "glue/create+train-r-interface2",
        .Platform$dynlib.ext,
        sep="")
dyn.load(OBJECTIVE.FUNCTION.SO)

if(!is.null(programOptions$features)) {
  features <- programOptions$features
} else {
  features <- "simple"  
}

if(!is.null(programOptions$lenpenalty.features)) {
  lenpenalty.features <- TRUE
}

# experimental scoring machine
if(!is.null(programOptions$noepshist.topology)) {
  .C("UseTopology_noepshist")
}

if(!is.null(programOptions$tlm)) {
  .C("UseTargetLM")
}

if(!is.null(programOptions$matrix.distance)) {
  .C("SetUseMatrixDistance")
}

if(joint.training == TRUE) {
  write("Joint objective function", stderr())
  objectiveFunctionType = 0;
} else {
  write("Conditional objective function", stderr())
  if(lenmatch){
    objectiveFunctionType = 2; # special length match objective
    write("Using lenmatch objective function", stderr())
  } else {
    objectiveFunctionType = 1; 
    write("(No lenmatch)", stderr())
  }
}

setLengthVariance <- function(variance){.C("SetLengthVariance", variance)}
setNumThreads <- function(n){.C("SetNumThreads", as.integer(n))}
setMemoryLimitInGb <- function(n){.C("SetMemoryLimitInGb", as.double(n))}
setMatchXY <- function(boolval){.C("SetMatchXY", boolval)}
setEigenvalueMaxiter <- function(m){.C("SetEigenvalueMaxiter", as.integer(m))}
setEigenvalueConvergenceTol <- function(t){.C("SetEigenvalueConvergenceTol", as.double(t))}
setSymCondProbThreshold <- function(t){.C("SetSymCondProbThreshold", as.double(t))}
setBackoffFunctions <- function(str){.C("SetBackoffFunctions", as.character(str))}

if(!is.null(programOptions$memory.limit)) {
  setMemoryLimitInGb(programOptions$memory.limit)
}

if(!is.null(programOptions$sym.threshold)) {
  setSymCondProbThreshold(programOptions$sym.threshold)
}

if(!is.null(backoff)) {
  setBackoffFunctions(backoff);
}

# for CheckConvergence
if(!is.null(programOptions$eigenvalue.maxiter)) {
  setEigenvalueMaxiter(programOptions$eigenvalue.maxiter)
}
if(!is.null(programOptions$eigenvalue.convergence.tol)) {
  setEigenvalueConvergenceTol(programOptions$eigenvalue.convergence.tol)
}

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
  if(!is.null(init.weights) && nchar(init.names) == 0) {
    stop("Error: You must specify the names of the features to initialize using --init-names")
  }
  write("Calling Init_FromAlignment", stderr());
  .C("Init_FromAlignment",
     as.integer(objectiveFunctionType),
     as.integer(ngramOrder),  
     as.integer(maxInsertions),  
     as.integer(numConjugations),  
     as.integer(numChangeRegions),  
     programOptions$align.fst,
     as.integer(doPrune),
     as.double(pruneWeightThreshold),
     as.double(pruneStateFactor),
     programOptions$train.data,
     programOptions$isymbols,
     programOptions$osymbols,
     features,
     init.names,
     as.double(variance));
} else {
  write(paste("# Model FST: all", programOptions$ngram.order, " grams"),
        stderr())
  if(!is.null(init.weights) && nchar(init.names) == 0) {
    write(paste("Error: You must specify the names of the features to initialize using --init-names",
                stderr()))
    stop()
  }
  .C("Init_Ngram",
     as.integer(objectiveFunctionType),
     as.integer(ngramOrder),
     as.integer(maxInsertions),
     programOptions$train.data,
     programOptions$isymbols,
     programOptions$osymbols,
     features,
     init.names,
     as.double(variance));
}

numParams = 1
numParams <- .C("GetNumParameters", result=integer(1))$result
write(paste("Found", numParams, "params"), stderr())

set.seed(1)
# initParams = rnorm(numParams, mean=0.0, sd=random.range)
minval <- -random.range
if(random.positive) {
  minval <- 0
}
initParams <- runif(numParams, min=minval, max=random.range)
if(!is.null(init.weights)) {
  write(paste("Reading", init.weights), stdout())
  paramsFromFile <- scan(init.weights, sep="\n")
  len <- length(paramsFromFile)
  initParams[1:len] <- paramsFromFile + runif(len, min=-init.weights.perturb,
                                              max=(init.weights.perturb))
  if(!is.null(programOptions$fst) && length(paramsFromFile) != numParams){
    write(paste("Warning: FST has ", numParams, " weights, but ",
                init.weights, " has ", length(paramsFromFile), " weights." ), stderr())
  }  
} 

if(!is.null(programOptions$lenpenalty.step)) {
  lenpenalty.step <- as.numeric(programOptions$lenpenalty.step)
}

if(force.convergence){
  if(!is.null(programOptions$lenpenalty.features)) {
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

if(!is.null(programOptions$num.threads)) {
  setNumThreads(programOptions$num.threads)
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

if(lbfgsMaxIterations > 0) {
  if(rprop.on) {
    write("# Optimization method: RPROP", stderr())
    #initParams <- .C("MakeConvergent", result=initParams, penaltyStepSize)$result
    result <- rprop(initParams, obj, gr, makeConverge, rprop.tolerance)
    #result <- list(par=initParams, value=100)
    
  } else {
    # control<-list(trace=6, REPORT=1, maxit=500)
    control<-list(trace=verboseLevel, REPORT=1, fnscale=1e-2,
                  maxit=lbfgsMaxIterations, factr=lbfgsConvergenceFactor)
    result <- optim(initParams, obj, gr, method="L-BFGS-B", control=control);
    # result <- optim(initParams, obj, NULL, method="L-BFGS-B", control=control);
    
    write(paste("Convergence: ", result$convergence), stderr())
    write(paste("Message: ", result$message), stderr())
  }
} else {
  write("Writing created FST and exiting. Have a good day.\n", stderr())
  result = list(par=initParams)
}

if(!is.null(programOptions$output)){
  filename <- paste(programOptions$output, ".feat-weights", sep="")
  write(paste("Writing", filename), stderr())
  write(result$par, file=filename, sep="\n")

  featureNamesFilename <- paste(programOptions$output, ".feat-names", sep="")
  write(paste("Writing", featureNamesFilename), stderr())
  .C("WriteFeatureNames", featureNamesFilename)

  fstName <- paste(programOptions$output, ".fst", sep="")
  .C("Save", fstName)
}

system("date")
.C("Cleanup")
