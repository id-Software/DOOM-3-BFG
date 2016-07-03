# Project: irrXML
# Makefile created by Dev-C++ 4.9.8.0

CPP  = g++
OBJ  = ../src/irrXML.o test.o 
LINKOBJ  = ../src/irrXML.o test.o 
LIBS =  --no-export-all-symbols --add-stdcall-alias
CXXINCS =  -I"../src" 
BIN  = example
CXXFLAGS = $(CXXINCS)

all: $(BIN)

clean: 
	rm -f $(OBJ) $(BIN)

$(BIN): $(LINKOBJ)
	$(CPP) $(LINKOBJ) -o $(BIN) $(LIBS)

../src/irrXML.o: ../src/irrXML.cpp
	$(CPP) -c ../src/irrXML.cpp -o ../src/irrXML.o $(CXXFLAGS)

test.o: test.cpp
	$(CPP) -c test.cpp -o test.o $(CXXFLAGS)
