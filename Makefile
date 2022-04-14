# preZ makefile

SRC = preZ.c src/*.c
EXE = preZ

STD = -std=c99
OPT = -O2
WFLAGS = -Wall -Wextra
INC = -I.

LDIR = lib
LIB = utopia xstring

LSTATIC = $(patsubst %,lib%.a,$(LIB))
LPATHS = $(patsubst %,$(LDIR)/%,$(LSTATIC))
LFLAGS = $(patsubst %,-L%,$(LDIR))
LFLAGS += $(patsubst %,-l%,$(LIB))

OS = $(shell uname -s)
ifeq ($(OS),Darwin)
    OSFLAGS = -mmacos-version-min=10.10
endif

CFLAGS = $(STD) $(OPT) $(WFLAGS) $(INC) $(OSFLAGS)

$(EXE): $(LPATHS) $(SRC) $(MAIN)
	$(CC) -o $@ $(SRC) $(MAIN) $(CFLAGS) $(LFLAGS)

$(LPATHS): $(LDIR) $(LSTATIC)
	mv *.a lib/

$(LDIR):
	mkdir $@

$(LDIR)%.a: %
	cd $^ && make && mv $@ ../

exe: $(SRC) $(MAIN)
	$(CC) -o $(EXE) $^ $(CFLAGS) $(LFLAGS)

clean: build.sh
	./$^ $@
