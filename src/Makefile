CC=cc
CV=$(shell if [ `uname` = "Darwin" ]; then echo "c11"; elif [ `uname` = "Linux" ]; then echo "gnu11"; fi;)
OS=$(shell echo `uname`)
ifeq ($(build),release)
	CFLAGS=-c -Wall -O3 -std=$(CV) -pedantic -D$(OS) -DOPO_TS -DHAVE_STDATOMIC_H
else
	CFLAGS=-c -Wall -g -Og -std=$(CV) -pedantic -D$(OS) -DOPO_TS -DHAVE_STDATOMIC_H
endif

SRC_DIR=.
INC_DIR=../include
LIB_DIR=../lib
LIB_DIRS=
INC_DIRS=-Iagoo

SRCS=$(wildcard agoo/*.c) $(wildcard *.c)
HEADERS=$(wildcard agoo/*.h) $(wildcard *.h)

OBJS=$(SRCS:.c=.o)
LIBS=-lm -lpthread
TARGET=$(LIB_DIR)/libagoo.a

all: $(INC_DIR) $(LIB_DIR) $(TARGET)
	rm -rf $(INC_DIR)/*
	cp *.h $(INC_DIR)
	mkdir $(INC_DIR)/agoo
	cp -r agoo/*.h $(INC_DIR)/agoo

clean:
	$(RM) *.o
	$(RM) -rf agoo/*.o
	$(RM) *~
	$(RM) .#*
	$(RM) $(TARGET)

$(INC_DIR):
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $@

$(TARGET): $(OBJS)
	ar -rcs $@ $(OBJS)

%.o : %.c $(HEADERS)
	$(DUMMY)
	$(CC) -I. $(INC_DIRS) $(CFLAGS) -o $@ $<
