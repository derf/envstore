prefix ?= /usr/local

build/envstore.1: bin/envstore
	mkdir -p build
	pod2man $< > $@

install: build/envstore.1
	mkdir -p $(prefix)/bin $(prefix)/share/man/man1
	cp bin/envstore $(prefix)/bin/envstore
	cp build/envstore.1 $(prefix)/share/man/man1/envstore.1

uninstall:
	rm -f $(prefix)/bin/envstore
	rm -f $(prefix)/share/man/man1/envstore

clean:
	rm -rf build

.PHONY: install uninstall clean
