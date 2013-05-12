ROOT=$(shell cd ../..; pwd)
include ../Makefile.include

TESTS=tagger lenmatch1 lenmatch2 insert-feature-weights # lenmatch-condother

.PHONY: $(TESTS)

all: $(TESTS)

tagger:
	$(TEST_START)
	$(MAKE) -C $(BIN_DIR) expectation-arc.so
	fstcompile --arc_type=expectation \
	  --isymbols=tagger/in.syms --osymbols=tagger/out.syms \
	  tagger/model.fst.txt > tagger/model.fst
	fstcompile --arc_type=expectation \
	  --isymbols=tagger/in.syms --osymbols=tagger/out.syms \
	  tagger/simple-model.fst.txt > tagger/simple-model.fst
	$(ROOT)/scripts/train.R --build.dir=$(BIN_DIR) \
	  --train-data=tagger/train-data \
	  --isymbols=tagger/in.syms --osymbols=tagger/out.syms \
	  --fst=tagger/model.fst
	$(ROOT)/scripts/train.R --build.dir=$(BIN_DIR) \
	  --train-data=tagger/train-data \
	  --isymbols=tagger/in.syms --osymbols=tagger/out.syms \
	  --fst=tagger/simple-model.fst
	$(TEST_END)

lenmatch1:
	$(TEST_START)
	fstcompile --arc_type=expectation lenmatch/len1.fst.txt \
	  > lenmatch/len1.fst
	fstcompile --arc_type=expectation lenmatch/len2.fst.txt \
	  > lenmatch/len2.fst
	$(BIN_DIR)/test-lenmatch lenmatch/len2.fst > lenmatch/1.stdout
	diff lenmatch/1.stdout lenmatch/1.stdout-expected && echo OK
	$(TEST_END)

lenmatch2:
	$(TEST_START)
	fstcompile --arc_type=expectation lenmatch/model.fst.txt \
          > lenmatch/model.fst
	$(ROOT)/scripts/train.R --build.dir=$(BIN_DIR) \
	  --train-data=lenmatch/train-data \
	  --isymbols=lenmatch/syms --osymbols=lenmatch/syms \
	  --fst=lenmatch/model.fst --lenmatch --lenmatch.variance=1 \
	  --output=lenmatch/out --random-range=0.0 --force-convergence
	diff lenmatch/out.feat-weights lenmatch/out.feat-weights-expected \
	  && echo OK
	rm lenmatch/out.fst
	$(TEST_END)

insert-feature-weights:
	$(TEST_START)
	fstcompile --arc_type=expectation insert-feature-weights/1.fst.txt \
	  > insert-feature-weights/1.fst
	fstcompile --arc_type=expectation insert-feature-weights/2.fst.txt \
	  > insert-feature-weights/2.fst
	$(BIN_DIR)/test-insert-feature-weights \
	  insert-feature-weights/1.fst insert-feature-weights/2.fst 
	$(TEST_END)

lenmatch-condother:
	$(TEST_START)
	fstcompile --isymbols=lenmatch-condother/0.syms --osymbols=lenmatch-condother/1.syms \
	  --arc_type=log lenmatch-condother/01.fst.txt > lenmatch-condother/01.fst
	fstcompile --isymbols=lenmatch-condother/0.syms --osymbols=lenmatch-condother/2.syms \
	  --arc_type=log lenmatch-condother/02.fst.txt > lenmatch-condother/02.fst
	fstcompile --isymbols=lenmatch-condother/1.syms --osymbols=lenmatch-condother/2.syms \
	  --arc_type=expectation lenmatch-condother/12.fst.txt > lenmatch-condother/12.fst
	$(ROOT)/scripts/train-condother.R \
	  --other-syms=lenmatch-condother/0.syms \
	  --other-other2in=lenmatch-condother/01.fst \
	  --other-other2out=lenmatch-condother/02.fst \
	  --other-list=lenmatch-condother/train-data.0 \
	  --train-data=lenmatch-condother/train-data \
	  --build.dir=$(BIN_DIR) \
	  --isymbols=lenmatch-condother/1.syms \
	  --osymbols=lenmatch-condother/2.syms \
	  --fst=lenmatch-condother/12.fst \
	  --lenmatch-condother \
	  --lenmatch-condother.variance=1 \
	  --output=lenmatch-condother/out \
	  --random-range=0.0 \
	  --variance=1e12 \
	  --kbest=0 \
	  --force-convergence
	diff lenmatch-condother/out.feat-weights \
	  lenmatch-condother/out.feat-weights-expected && echo OK
	$(TEST_END)
