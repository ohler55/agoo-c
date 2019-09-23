CC=cc
CV=$(shell if [ `uname` = "Darwin" ]; then echo "c11"; elif [ `uname` = "Linux" ]; then echo "gnu11"; fi;)
OS=$(shell echo `uname`)
CFLAGS=-c -Wall -O3 -std=$(CV) -pedantic -D$(OS)
#CFLAGS=-c -Wall -g -Og -std=$(CV) -pedantic -D$(OS)

SSL_INC=
ifneq (,$(wildcard /usr/local/opt/openssl/include/openssl/ssl.h))
	SSL_INC=-I/usr/local/opt/openssl/include
	CFLAGS+= -DHAVE_OPENSSL_SSL_H
else ifneq (,$(wildcard /usr/local/openssl/include/openssl/ssl.h))
	SSL_INC=-I/usr/local/openssl/include
	CFLAGS+= -DHAVE_OPENSSL_SSL_H
endif

SSL_LIB=
SSL_LIB_DIR=
ifneq (,$(wildcard /usr/local/opt/openssl/lib/libssl.a))
	SSL_LIB_DIR=-L/usr/local/opt/openssl/lib
	SSL_LIB=-lssl -lcrypto
else ifneq (,$(wildcard /usr/local/openssl/lib/libssl.a))
	SSL_LIB_DIR=-L/usr/local/openssl/lib
	SSL_LIB=-lssl -lcrypto
endif


SRC_DIR=.
LIB_DIRS=-L../../lib $(SSL_LIB_DIR)
INC_DIRS=-I../../include $(SSL_INC)

SRCS=$(shell find $(SRC_DIR) -type f -name "*.c" -print)
LIBS=-lagoo $(SSL_LIB) -lpthread -lm
OBJS=$(SRCS:.c=.o)
TARGET=simple

all: $(TARGET)

clean:
	$(RM) *.o
	$(RM) *~
	$(RM) .#*
	$(RM) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LIB_DIRS) $(LIBS)

%.o : %.c
	$(CC) -I. $(INC_DIRS) $(CFLAGS) -o $@ $<

test: $(TARGET)
	$(TARGET)
