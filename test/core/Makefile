ROOT=$(shell cd ../..; pwd)
include ../Makefile.include

TESTS=construct neg-log-of-signed-num construct-fstprint fst-bin

all: $(TESTS)

fst-bin:
	$(TEST_START)
	$(MAKE) -C $(BIN_DIR) expectation-arc.so
	fstcompile --acceptor --arc_type=expectation expectation-arc-fire1+1+1+1+1.fst.txt \
	  > expectation-arc-fire1+1+1+1+1.fst
	fstcompile --acceptor --arc_type=expectation expectation-arc-fire2+2+1.fst.txt \
	  > expectation-arc-fire2+2+1.fst
	fstcompile --acceptor --arc_type=expectation expectation-arc-fire2+3.fst.txt \
	  > expectation-arc-fire2+3.fst
	fstprint expectation-arc-fire1+1+1+1+1.fst > fst-bin.stdout
	echo >> fst-bin.stdout
	fstprint expectation-arc-fire2+2+1.fst >> fst-bin.stdout
	echo >> fst-bin.stdout
	fstprint expectation-arc-fire2+3.fst >> fst-bin.stdout
	diff fst-bin.stdout fst-bin.stdout-expected && echo OK
	$(TEST_END)

construct: 
	$(TEST_START)
	$(MAKE) -C $(BIN_DIR) test-construct
	$(BIN_DIR)/test-construct > construct.fst
	$(TEST_END)

construct-fstprint: construct 
	$(TEST_START)
	$(MAKE) -C $(BIN_DIR) expectation-arc.so
	fstprint construct.fst > construct.stdout
	diff construct.stdout construct.stdout-expected && echo OK
	$(TEST_END)

neg-log-of-signed-num: 
	$(TEST_START)
	$(MAKE) -C $(BIN_DIR) test-neg-log-of-signed-num
	$(BIN_DIR)/test-neg-log-of-signed-num
	$(TEST_END)

