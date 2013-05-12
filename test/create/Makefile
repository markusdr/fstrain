ROOT=$(shell cd ../..; pwd)
include ../Makefile.include

.PHONY: test-create-1 test-backoff

all: test-create-1 test-backoff

test-create-1: $(BIN_DIR)/test-create-1 # $(BIN_DIR)/features
	$(BIN_DIR)/test-create-1 
#	$(BIN_DIR)/features

test-backoff: $(BIN_DIR)/test-backoff
	fstcompile --isymbols=backoff/ngrams1.syms --acceptor --arc_type=expectation backoff/ngrams1.fst.txt > backoff/ngrams1.fst 
	fstcompile --arc_type=expectation backoff/ngrams2.fst.txt > backoff/ngrams2.fst 
	$(BIN_DIR)/test-backoff --symbols=backoff/ngrams2.syms --fst=backoff/ngrams2.fst 

test-custom-phi: $(BIN_DIR)/test-custom-phi-matcher
	$(TEST_START)
	fstcompile --arc_type=log --acceptor --isymbols=custom-phi/1.syms \
	  custom-phi/1.fst.txt > custom-phi/1.fst
	$(BIN_DIR)/test-custom-phi-matcher \
	  --symbols=custom-phi/1.syms \
	  --fst=custom-phi/1.fst > custom-phi/1.stdout
	diff custom-phi/1.stdout custom-phi/1.stdout-expected && echo OK
	$(TEST_END)
