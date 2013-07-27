CXX      ?= g++
AR       ?= ar
CXXFLAGS ?= -O2 -g3 -Werror
CXXFLAGS += -std=c++11 -Wall -Wshadow

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
BRIGADESSRCFILES = Side.cpp Armor.cpp Road.cpp Terrain.cpp World.cpp Soldier.cpp \
		   SoldierQuery.cpp WeaponQuery.cpp \
		   SoldierController.cpp SoldierAgent.cpp PlayerAgent.cpp ai/SoldierAgent.cpp \
		   Armory.cpp SensorySystem.cpp Trigger.cpp Event.cpp \
		   InputState.cpp AgentDirectory.cpp SoldierAction.cpp Driver.cpp \
		   InfoChannel.cpp DebugOutput.cpp main.cpp

BRIGADESSRCS = $(addprefix $(BRIGADESSRCDIR)/, $(BRIGADESSRCFILES))
BRIGADESOBJS = $(BRIGADESSRCS:.cpp=.o)
BRIGADESDEPS = $(BRIGADESSRCS:.cpp=.dep)


.PHONY: clean all

all: $(BRIGADESBIN)

$(BINDIR):
	mkdir -p $(BINDIR)

COMMONDIR = src/common
COMMONSRCS = $(shell (find $(COMMONDIR) \( -name '*.cpp' -o -name '*.h' \)))

$(COMMONLIB): $(COMMONSRCS)
	make -C $(COMMONDIR)

$(BRIGADESBIN): $(COMMONLIB) $(BRIGADESOBJS) $(BINDIR)
	$(CXX) $(LDFLAGS) $(BRIGADESLIBS) $(BRIGADESOBJS) $(COMMONLIB) -o $(BRIGADESBIN)

%.dep: %.cpp
	@rm -f $@
	@$(CXX) -MM $(CXXFLAGS) $< > $@.P
	@sed 's,\($(notdir $*)\)\.o[ :]*,$(dir $*)\1.o $@ : ,g' < $@.P > $@
	@rm -f $@.P

clean:
	find src/ -name '*.o' -exec rm -rf {} +
	find src/ -name '*.dep' -exec rm -rf {} +
	find src/ -name '*.a' -exec rm -rf {} +
	rm -rf $(BRIGADESBIN)
	rmdir $(BINDIR)

-include $(BRIGADESDEPS)

