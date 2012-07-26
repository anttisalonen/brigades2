CXX      ?= g++
AR       ?= ar
CXXFLAGS ?= -std=c++11 -O2 -g3 -Werror
CXXFLAGS += -Wall

CXXFLAGS += $(shell sdl-config --cflags)

BRIGADESLIBS = $(shell sdl-config --libs) -lSDL_image -lSDL_ttf -lGL -lboost_serialization -lboost_iostreams

CXXFLAGS += -Isrc
BINDIR       = bin

# Common lib

COMMONSRCDIR = src/common
COMMONLIB = $(COMMONSRCDIR)/libcommon.a

# Brigades

BRIGADESBINNAME = brigades
BRIGADESBIN     = $(BINDIR)/$(BRIGADESBINNAME)
BRIGADESSRCDIR = src/brigades
BRIGADESSRCFILES = main.cpp

BRIGADESSRCS = $(addprefix $(BRIGADESSRCDIR)/, $(BRIGADESSRCFILES))
BRIGADESOBJS = $(BRIGADESSRCS:.cpp=.o)
BRIGADESDEPS = $(BRIGADESSRCS:.cpp=.dep)


.PHONY: clean all

all: $(BRIGADESBIN)

$(BINDIR):
	mkdir -p $(BINDIR)

$(COMMONLIB):
	make -C src/common

$(BRIGADESBIN): $(BINDIR) $(BRIGADESOBJS) $(COMMONLIB)
	$(CXX) $(LDFLAGS) $(BRIGADESOBJS) $(COMMONLIB) -o $(BRIGADESBIN)

%.dep: %.cpp
	@rm -f $@
	@$(CC) -MM $(CXXFLAGS) $< > $@.P
	@sed 's,\($(notdir $*)\)\.o[ :]*,$(dir $*)\1.o $@ : ,g' < $@.P > $@
	@rm -f $@.P

clean:
	find src/ -name '*.o' -exec rm -rf {} +
	find src/ -name '*.dep' -exec rm -rf {} +
	find src/ -name '*.a' -exec rm -rf {} +
	rm -rf $(BRIGADESBIN)
	rmdir $(BINDIR)

-include $(BRIGADESDEPS)

