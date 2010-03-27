CFLAGS ?= -O2
PREFIX ?= /usr/local

CFLAGS += -Wall -Wextra -pedantic

sysdir = ${DESTDIR}${PREFIX}

all: bin/envstore

bin/%: src/%.c
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $<

install: bin/envstore
	@echo installing:
	@echo 'envstore   to ${sysdir}/bin'
	@echo 'envify     to ${sysdir}/bin'
	@echo 'envstore.1 to ${sysdir}/share/man/man1'
	@echo 'envify.1   to ${sysdir}/share/man/man1'
	@mkdir -p ${sysdir}/bin ${sysdir}/share/man/man1
	@cp bin/envstore ${sysdir}/bin/envstore
	@cp bin/envify ${sysdir}/bin/envify
	@cp man/1/envify ${sysdir}/share/man/man1/envify.1
	@cp man/1/envstore ${sysdir}/share/man/man1/envstore.1
	@chmod 755 ${sysdir}/bin/envstore
	@chmod 755 ${sysdir}/bin/envify
	@chmod 644 ${sysdir}/share/man/man1/envify.1
	@chmod 644 ${sysdir}/share/man/man1/envstore.1

uninstall:
	rm -f ${sysdir}/bin/envstore ${sysdir}/bin/envify
	rm -f ${sysdir}/share/man/man1/envify.1
	rm -f ${sysdir}/share/man/man1/envstore.1

test:
	test/main

clean:
	rm -f bin/envstore

.PHONY: all install uninstall test clean
