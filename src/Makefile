CC=cc
CV=$(shell if [ `uname` = "Darwin" ]; then echo "c11"; elif [ `uname` = "Linux" ]; then echo "gnu11"; fi;)
OS=$(shell echo `uname`)
ifeq ($(build),release)
	CFLAGS=-c -Wall -O3 -std=$(CV) -pedantic -D$(OS) -DOPO_TS
else
	CFLAGS=-c -Wall -g -Og -std=$(CV) -pedantic -D$(OS) -DOPO_TS
endif

# Create first so SRCS get setup with the agoo files.
AGOO=agoo
AGOO_SRC=../../agoo/ext/agoo
ifeq "$(wildcard $(AGOO))" ""
	DUMMY=$(shell mkdir -p $(AGOO) && \
		cp $(AGOO_SRC)/base64.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/bind.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/con.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/debug.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/doc.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/dtime.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/err.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/graphql.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/gqlintro.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/gqlvalue.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/hook.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/http.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/kinds.h $(AGOO) &&\
		cp $(AGOO_SRC)/log.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/method.h $(AGOO) &&\
		cp $(AGOO_SRC)/page.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/pub.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/queue.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/req.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/res.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/respponse.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/sha1.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/seg.h $(AGOO) &&\
		cp $(AGOO_SRC)/server.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/sdl.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/sse.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/subject.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/text.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/upgraded.[ch] $(AGOO) &&\
		cp $(AGOO_SRC)/websocket.[ch] $(AGOO)\
	)
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
	rsync -tupRE $(HEADERS) $(INC_DIR)

clean:
	$(RM) *.o
	$(RM) *~
	$(RM) .#*
	$(RM) $(TARGET)
	$(RM) -rf $(AGOO)

refresh:
	mkdir -p $(AGOO)
	echo "all:\n	make -C ..\n" > agoo/Makefile
	cp ../../agoo/ext/agoo/base64.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/bind.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/con.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/debug.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/doc.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/dtime.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/err.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/graphql.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/gqlintro.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/gqlvalue.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/hook.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/http.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/kinds.h $(AGOO)
	cp ../../agoo/ext/agoo/log.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/method.h $(AGOO)
	cp ../../agoo/ext/agoo/page.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/pub.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/queue.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/req.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/res.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/response.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/sha1.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/seg.h $(AGOO)
	cp ../../agoo/ext/agoo/server.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/sdl.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/sse.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/subject.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/text.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/upgraded.[ch] $(AGOO)
	cp ../../agoo/ext/agoo/websocket.[ch] $(AGOO)

$(INC_DIR):
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $@

$(TARGET): $(OBJS)
	ar -rcs $@ $(OBJS)

%.o : %.c $(HEADERS)
	$(DUMMY)
	$(CC) -I. $(INC_DIRS) $(CFLAGS) -o $@ $<
