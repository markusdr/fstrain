.PHONY: test doc pack install

DOXYGEN=doxygen

test:
	$(MAKE) -C test

doc:
	$(DOXYGEN) doc/doxygen.cfg

-include Makefile.devel
