
all clean: lib 
	cd src; $(MAKE) $@
	cd test; $(MAKE) $@
	cd examples; $(MAKE) $@

lib:
	mkdir -p $@

