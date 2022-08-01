include /usr/local/etc/PcapPlusPlus.mk

IDIR = include
CXX := g++
CXXFLAGS := -std=c++17
srcfiles := $(shell find . -name "*.cpp")
objects  := $(patsubst %.cpp, %.o, $(srcfiles))
# All Target
all: $(threadedreader)

$(threadedreader): $(objects)
	$(CXX) $(CXXFLAGS) $(PCAPPP_INCLUDES) -o $(threadedreader) $(objects) $(PCAPPP_LIBS_DIR) 

depend: .depend

.depend: $(srcfiles)
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM $^>>./.depend;

clean:
	rm -f $(objects)

dist-clean: clean
	rm -f *~ .depend

include .depend
