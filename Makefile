PREFIX = ${DESTDIR}/usr
BINDIR = ${PREFIX}/sbin
BINDIR_ALT = ${PREFIX}/bin

LIBS = libevent libcrypto libbsd libldns

CFLAGS = -Wall -D_GNU_SOURCE `pkg-config --cflags ${LIBS}`
LDFLAGS = -Wl,--as-needed `pkg-config --libs ${LIBS}`

OBJ_CLIENT = src/dnstress.o src/argparser.o src/utils.o \
			 src/jsmn.o src/dnsconfig.o src/worker.o \
			 src/threadpool.o src/tcp.o src/udp.o

CPPFLAGS = -DDEBUG

all: dnstress

dnstress: ${OBJ_CLIENT}
	${CC} ${OBJ_CLIENT} ${LDFLAGS} -lpthread -o dnstress

clean:
	rm -f dnstress \
	${OBJ_CLIENT}