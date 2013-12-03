CC=g++
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=main.cpp
TESTSOURCES=test.cpp
OBJECTS=$(SOURCES:.cpp=.o)
TESTOBJECTS=$(TESTSOURCES:.cpp=.o)
EXECUTABLE=PartOne
TESTEXECUTABLE=PartOneTest
TESTLDFLAGS=-lgtest_main -lgtest -lpthread
TESTINCLUDE=-I"/home/hue/git/Project-6/PartOne/gtest-1.7.0/include"
TESTLINK=-L"/home/hue/git/Project-6/PartOne/gtest-1.7.0"

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

test: $(TESTSOURCES) $(TESTEXECUTABLE)

$(TESTEXECUTABLE): $(TESTOBJECTS)
	$(CC) $(TESTLINK) $(TESTOBJECTS) -o $@ $(TESTLDFLAGS)  

%.o: %.cpp
	$(CC) $(TESTINCLUDE) $(CFLAGS) -o $@ $<
