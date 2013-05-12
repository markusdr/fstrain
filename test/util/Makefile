ROOT=$(shell cd ../..; pwd)
include ../Makefile.include

TESTS = test-determinized-union1 test-determinized-union2 test-determinized-union3 \
          test-determinized-union4

.PHONY: $(TESTS)

all: $(TESTS)

test-determinized-union1: $(BIN_DIR)/test-determinized-union
	$(TEST_START)
	fstcompile --arc_type=log --acceptor \
	  determinized-union/a1.fst.txt > determinized-union/a1.fst
	fstcompile --arc_type=log --acceptor \
	  determinized-union/a2.fst.txt > determinized-union/a2.fst
	$(BIN_DIR)/test-determinized-union \
	  determinized-union/a1.fst determinized-union/a2.fst \
	  > determinized-union/a-result.fst.txt
	fstcompile --arc_type=log determinized-union/a-result.fst.txt > determinized-union/a-result.fst
	./determinized-union/detunion.sh determinized-union/a1.fst determinized-union/a2.fst \
	  | fstequivalent - determinized-union/a-result.fst && echo OK
	$(BIN_DIR)/test-determinized-union \
	  determinized-union/a2.fst determinized-union/a1.fst \
	  > determinized-union/a-result2.fst.txt
	fstcompile --arc_type=log determinized-union/a-result2.fst.txt > determinized-union/a-result2.fst
	fstequivalent determinized-union/a-result.fst determinized-union/a-result2.fst && echo OK
	$(TEST_END)

test-determinized-union2: $(BIN_DIR)/test-determinized-union
	$(TEST_START)
	fstcompile --arc_type=log --acceptor \
	  determinized-union/b1.fst.txt > determinized-union/b1.fst
	$(BIN_DIR)/test-determinized-union \
	  determinized-union/b1.fst determinized-union/b1.fst \
	  > determinized-union/b-result1.fst.txt
	fstcompile --arc_type=log determinized-union/b-result1.fst.txt > determinized-union/b-result1.fst
	./determinized-union/detunion.sh determinized-union/b1.fst determinized-union/b1.fst \
	  | fstequivalent - determinized-union/b-result1.fst && echo OK
	$(TEST_END)

test-determinized-union3: test-determinized-union2
	$(TEST_START)
	$(BIN_DIR)/test-determinized-union \
	  determinized-union/b-result1.fst determinized-union/b1.fst \
	  > determinized-union/b-result2.fst.txt
	fstcompile --arc_type=log determinized-union/b-result2.fst.txt > determinized-union/b-result2.fst
	./determinized-union/detunion.sh determinized-union/b1.fst determinized-union/b1.fst \
	  > determinized-union/b-result3.fst
	./determinized-union/detunion.sh determinized-union/b-result3.fst determinized-union/b1.fst \
	  | fstequivalent - determinized-union/b-result2.fst && echo OK
	$(TEST_END)

# It won't work in this case; the alg really only works for tries
# test-determinized-union4: $(BIN_DIR)/test-determinized-union
# 	$(TEST_START)
# 	fstcompile --arc_type=log --acceptor \
# 	  determinized-union/c1.fst.txt > determinized-union/c1.fst
# 	fstcompile --arc_type=log --acceptor \
# 	  determinized-union/c2.fst.txt > determinized-union/c2.fst
# 	$(BIN_DIR)/test-determinized-union \
# 	  determinized-union/c1.fst determinized-union/c2.fst \
# 	  > determinized-union/c-result.fst.txt
# 	fstcompile --arc_type=log determinized-union/c-result.fst.txt > determinized-union/c-result.fst
# 	./determinized-union/detunion.sh determinized-union/c1.fst determinized-union/c2.fst \
# 	  | fstequivalent - determinized-union/c-result.fst && echo OK
# 	$(TEST_END)
