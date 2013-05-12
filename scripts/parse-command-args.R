# Parses a vector of options, e.g. --width=23 --test --foo=bar --screen-width=23
# Returns a list, e.g. list(width="23, test=TRUE, foo="bar", screen.width="23")
parseCommandArgs <- function() {
  options <- list()
  for (e in commandArgs(TRUE)) {
    ta = strsplit(e, "=", fixed=TRUE)
    pair = ta[[1]]
    key = pair[1]
    if(substr(key, 0, 2) == "--") {
      key = substr(key, 3, nchar(key))
      key = gsub('-', '.', key)
      if(is.na(pair[2])) {
        options[[key]] = TRUE
      } else {
        options[[key]] = pair[2]
      }
    }
    else {
      print(paste("Options error:", ta));
      quit()
    }
  }
  options
  return(options)
}
