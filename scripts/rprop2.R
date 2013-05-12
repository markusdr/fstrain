#!/usr/bin/rscript

rprop <- function(params, func, grad, makeConverge, abstol=1e-3) {
  maxiter <- 100
  updates <- rep(0.5, length(params))
  prevGradients <- rep(0.0, length(params))
  prevGradientsMean <- 0.0
  eta.minus <- 0.5
  eta.plus <- 1.2
  updateMin <- 1e-6
  updateMax <- 50
  funval <- Inf
  for(iter in 1:maxiter) {
    write(cat("rprop iter", iter), stderr())
    funval <- func(params)
    gradients <- grad(params)
    len <- length(gradients)
    for(i in 1:len) {
      gradientProduct <- prevGradients[i] * gradients[i]
      if(gradientProduct > 0){ # no sign change
        updates[i] <- min(updates[i] * eta.plus, updateMax)
        delta <- -sign(gradients[i]) * updates[i]
        params[i] <- params[i] + delta
        prevGradients[i] <- gradients[i]
      } else if(gradientProduct < 0) {
        updates[i] <- max(updates[i] * eta.minus, updateMin)
        prevGradients[i] <- 0
      } else {
        delta <- -sign(gradients[i]) * updates[i]
        params[i] <- params[i] + delta
        prevGradients[i] <- gradients[i]
      } 
    }
    updatesMean <- mean(updates)
    write(cat("rprop updates mean=", updatesMean), stderr())
    if(updatesMean < abstol){
      write("converged", stderr())
      break
    }
  }
  return(list(par=params, value=funval))
}

