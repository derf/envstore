prefix ?= /usr/local

manuals: build/envstore.1 build/envify.1

build/%.1: man/1/%.pod
	mkdir -p build
	pod2man $< > $@

install: manuals
	mkdir -p $(prefix)/bin $(prefix)/share/man/man1
	cp bin/envstore $(prefix)/bin/envstore
	cp bin/envify $(prefix)/bin/envify
	cp build/envstore.1 $(prefix)/share/man/man1/envstore.1
	cp build/envify.1 $(prefix)/share/man/man1/envify.1
	chmod 755 $(prefix)/bin/envstore
	chmod 755 $(prefix)/bin/envify
	chmod 644 $(prefix)/share/man/man1/envstore.1
	chmod 644 $(prefix)/share/man/man1/envify.1

uninstall:
	rm -f $(prefix)/bin/envstore
	rm -f $(prefix)/bin/envify
	rm -f $(prefix)/share/man/man1/envstore.1
	rm -f $(prefix)/share/man/man1/envify.1

clean:
	rm -rf build

.PHONY: install manuals uninstall clean
