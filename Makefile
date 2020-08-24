OUTPUT=output.png
SVGINPUT=circle.svg
PROGRAM=d2d-test.exe
OBJECTS=d2d-test.o
CPPFLAGS=-DNO_SVG
CFLAGS=-municode -O0 -ggdb3 -Wall -Wextra -Werror
CXXFLAGS=
LDFLAGS=
LDLIBS=-ld2d1 -lwindowscodecs -lole32 -lshlwapi
CXX=g++

all: $(OUTPUT)

clean:
	-del $(OUTPUT) $(PROGRAM) $(OBJECTS)

$(OUTPUT): $(PROGRAM) $(SVGINPUT)
	$(PROGRAM) $(OUTPUT) $(SVGINPUT)

%.o: %.cpp
	$(CXX) -c -o $@ $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) $<

$(PROGRAM): $(OBJECTS)
	$(CXX) -o $@ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS)

.PHONY: all clean
