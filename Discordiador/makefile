PROCESS_NAME=Discordiador
IDIR =./include
SHARED_IDIR = ../shared/include
CC=gcc
CFLAGS=-I$(IDIR) -I$(SHARED_IDIR) -g -Wall

CFG=./cfg
ODIR=./obj
SRCDIR =./src
LIBS=-lcommons -lpthread -lreadline -lcunit -lrt

SHARED_SRCDIR = ../shared/src
SHARED_ODIR = ../shared/obj

DEPS = $(wildcard $(IDIR)/*.h)

SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c,$(ODIR)/%.o,$(SRC))

all: DIRECTORIO_OBJ_CFG $(PROCESS_NAME)

DIRECTORIO_OBJ_CFG:
	mkdir -p $(ODIR) $(CFG) $(SHARED_ODIR)

$(ODIR)/%.o: $(SRCDIR)/%.c $(DEPS) $(SHARED_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(SHARED_ODIR)/%.o: $(SHARED_SRCDIR)/%.c $(SHARED_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

SHARED_DEPS = $(wildcard $(SHARED_IDIR)/*.h)

SHARED_SRC = $(wildcard $(SHARED_SRCDIR)/*.c)
SHARED_OBJ = $(patsubst $(SHARED_SRCDIR)/%.c,$(SHARED_ODIR)/%.o,$(SHARED_SRC))

$(PROCESS_NAME): $(OBJ) $(SHARED_OBJ)
	$(CC) -o $@.out $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ $(PROCESS_NAME).out $(SHARED_ODIR)/*.o 