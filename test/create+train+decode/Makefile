ROOT=$(shell cd ../..; pwd)
include ../Makefile.include

.PHONY: letters chunker

# TESTS=small letters chunker
TESTS= letters chunker

all: $(TESTS)

small:
	$(TEST_START)
	cd $(ROOT); \
	  ./scripts/train+test.pl --train-data=test/create+train+decode/1.train \
	  --test-data=test/create+train+decode/1.test \
	  --isymbols=test/create+train+decode/ascii.syms \
	  --osymbols=test/create+train+decode/ascii.syms \
	  --output=test/create+train+decode/foo \
	  --max-iterations=50
	cat foo.test.stderr
	diff foo.test.stderr foo.test.stderr-expected && echo OK
#	rm foo.train-align.stdout \
#	  foo.train.stdout \
#	  foo.fst \
#	  foo.feat-weights \
#	  foo-align.fst \
#	  foo-align.feat-weights
	$(TEST_END)

letters:
	$(TEST_START)
	cd $(ROOT); \
	./scripts/train.R \
          --isymbols=test/create+train+decode/letters/syms \
          --osymbols=test/create+train+decode/letters/syms \
          --train-data=test/create+train+decode/letters/train \
          --ngram-order=1 \
	  --features=simple \
          --force-convergence \
          --output=test/create+train+decode/letters/out
	$(BIN_DIR)/drivers/transducer-decode \
	  --fst=letters/out.fst \
	  --isymbols=letters/syms --osymbols=letters/syms letters/train \
	  > letters/out.decoded
	diff letters/out.decoded letters/out.decoded-expected && echo OK
	cd $(ROOT); \
	./scripts/train.R \
          --isymbols=test/create+train+decode/letters/syms \
          --osymbols=test/create+train+decode/letters/syms \
          --train-data=test/create+train+decode/letters/train \
          --ngram-order=2 \
	  --features=simple+len \
          --force-convergence --lenpenalty-features \
          --align-fst=test/create+train+decode/letters/out.fst \
          --init-weights=test/create+train+decode/letters/out.feat-weights \
          --init-names=test/create+train+decode/letters/out.feat-names \
          --output=test/create+train+decode/letters/out2

	$(BIN_DIR)/transducer-decode \
	  --fst=letters/out2.fst \
	  --isymbols=letters/syms --osymbols=letters/syms letters/train \
	  > letters/out2.decoded
	diff letters/out2.decoded letters/out2.decoded-expected && echo OK
	$(TEST_END)

chunker:
	$(TEST_START)
	cd $(ROOT); \
	./scripts/train.R \
          --isymbols=test/create+train+decode/chunker/isyms \
          --osymbols=test/create+train+decode/chunker/osyms \
          --train-data=test/create+train+decode/chunker/train \
          --ngram-order=2 \
          --align-fst=simple \
          --output=test/create+train+decode/chunker/out
	$(BIN_DIR)/transducer-decode \
	  --fst=chunker/out.fst --evaluate=false \
	  --isymbols=chunker/isyms --osymbols=chunker/osyms chunker/train \
	  > chunker/out.decoded
	diff chunker/out.decoded chunker/out.decoded-expected && echo OK
	$(TEST_END)
