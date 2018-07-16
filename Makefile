PREFIX = ${DESTDIR}/usr
BINDIR = ${PREFIX}/sbin
BINDIR_ALT = ${PREFIX}/bin

LIBS = libevent libcrypto libbsd libldns

CFLAGS = -Wall -D_GNU_SOURCE `pkg-config --cflags ${LIBS}`
LDFLAGS = -Wl,--as-needed `pkg-config --libs ${LIBS}`

OBJ_CLIENT = dnstress.o argparser.o utils.o jsmn.o dnsconfig.o

CPPFLAGS = -DDEBUG

all: dnstress

dnstress: ${OBJ_CLIENT}
	${CC} ${OBJ_CLIENT} ${LDFLAGS} -o dnstress

clean:
	rm -f dnstress \
	${OBJ_CLIENT}