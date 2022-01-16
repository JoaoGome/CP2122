# source files.
SRC = Paralela.c

OBJ = $(SRC:.cpp=.o)

OUT = p

# include directories
INCLUDES = -I.

# C compiler flags
CCFLAGS = -std=c99 -O2 -fopenmp -lm -Wall -I/share/apps/papi/5.4.1/include

# compiler
CCC = gcc

# libraries
LIBS = -L/share/apps/papi/5.4.1/lib -lm -lpapi

.SUFFIXES: .cpp .c


default: $(OUT)

.cpp.o:
        $(CCC) $(CCFLAGS) $(INCLUDES)  -c $< -o $@

.c.o:
        $(CCC) $(CCFLAGS) $(INCLUDES) -c $< -o $@

$(OUT): $(OBJ)
        $(CCC) -o $(OUT) $(CCFLAGS) $(OBJ) $(LIBS)

depend:  dep
#
#dep:
#       makedepend -- $(CFLAGS) -- $(INCLUDES) $(SRC)

clean:
        rm -f *.o .a *~ Makefile.bak $(OUT)
