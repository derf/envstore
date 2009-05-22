prefix ?= /usr/local

build/envstore.1: bin/envstore
	mkdir -p build
	pod2man $< > $@

install: build/envstore.1
	install -m 755 -D bin/envstore $(prefix)/bin/envstore
	install -m 644 -D build/envstore.1 $(prefix)/share/man/man1/envstore.1

uninstall:
	$(RM) $(prefix)/bin/envstore
	$(RM) $(prefix)/share/man/man1/envstore

clean:
	$(RM) -r build

.PHONY: install uninstall clean
