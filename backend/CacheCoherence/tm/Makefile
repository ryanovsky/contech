PROJECT = ccache
CXX=g++
CXXFLAGS= -g -std=c++11 -O3
OBJECTS= main.o memory.o simpleCache.o graphTraverse.o cacheCoherence.o bus.o splitBus.o time.o
INCLUDES= -I../../../common/taskLib/
LIBS= -L../../../common/taskLib/ -lTask -lz -L.
EXTERNALS=

all: taskLib $(PROJECT)

taskLib:
	$(MAKE) -C ../../../common/taskLib

%.o : %.cpp
	$(CXX) $< $(CXXFLAGS) $(INCLUDES) -c -o $@
$(PROJECT): $(OBJECTS)
	$(CXX) -o $(PROJECT) $(OBJECTS) $(CXXFLAGS) $(LIBS)

clean:
	rm $(OBJECTS) $(PROJECT)

