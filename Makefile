CFLAGS = -Wall -Wextra -pedantic -O2
prefix = /usr/local

all: bin/envstore

bin/%: src/%.c
	$(CC) $(CFLAGS) -o $@ $<

install: bin/envstore
	mkdir -p $(prefix)/bin $(prefix)/share/man/man1
	cp bin/envstore $(prefix)/bin/envstore
	cp bin/envify $(prefix)/bin/envify
	cp man/1/envify $(prefix)/share/man/man1/envify.1
	cp man/1/envstore $(prefix)/share/man/man1/envstore.1
	chmod 755 $(prefix)/bin/envstore
	chmod 755 $(prefix)/bin/envify
	chmod 644 $(prefix)/share/man/man1/envify.1
	chmod 644 $(prefix)/share/man/man1/envstore.1

uninstall:
	rm -f $(prefix)/bin/envstore $(prefix)/bin/envify
	rm -f $(prefix)/share/man/man1/envify.1
	rm -f $(prefix)/share/man/man1/envstore.1

test:
	zsh test/main --extended

clean:
	rm -f bin/envstore

.PHONY: all install uninstall test clean
