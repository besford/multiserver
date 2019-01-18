##----------------------------------------------------------------------------##
## Variables                                                                  ##
##----------------------------------------------------------------------------##

PROGRAM		= server

EXTENSION	= c
OBJECT		= Object/
SOURCE		= Source/

CXX			= gcc
LIBRARIES	= -lpthread

ifeq ($(mode), release)
	CXXFLAGS = -Wall -pedantic-errors -O2 -s
else
	mode     = debug
	CXXFLAGS = -Wall -pedantic-errors -O0 -g
endif

SOURCES		= $(shell find $(SOURCE) -name "*.$(EXTENSION)")
OBJECTS		= $(shell find $(OBJECT) -name "*.o")
GENERATED	= $(patsubst $(SOURCE)%.$(EXTENSION), $(OBJECT)%.o, $(SOURCES))



##----------------------------------------------------------------------------##
## Compile                                                                    ##
##----------------------------------------------------------------------------##

makepp_no_builtin = 1

$(OBJECT)%.o: $(SOURCE)%.$(EXTENSION)
	$(CXX) $(CXXFLAGS) -c $< -o $@



##----------------------------------------------------------------------------##
## Build                                                                      ##
##----------------------------------------------------------------------------##

.PHONY: build rebuild clean

build: _build $(GENERATED)
	$(CXX) $(OBJECTS) -o $(PROGRAM) $(LIBRARIES)
	@echo Build succeeded

rebuild: _rebuild clean $(GENERATED)
	$(CXX) $(OBJECTS) -o $(PROGRAM) $(LIBRARIES)
	@echo Build succeeded

clean:
	$(RM) $(PROGRAM) $(GENERATED) $(PROGRAM).tar.gz



##----------------------------------------------------------------------------##
## Run                                                                        ##
##----------------------------------------------------------------------------##

.PHONY: debug run leaktest

debug: build
	gdb ./$(PROGRAM)

run: build
	./$(PROGRAM)

leaktest: build
	valgrind --leak-check=full -v --show-reachable=yes ./$(PROGRAM)



##----------------------------------------------------------------------------##
## Helpers                                                                    ##
##----------------------------------------------------------------------------##

.PHONY: clear tar readme cloc

clear:
	clear

tar: clear clean
	@echo Creating tar file: $(PROGRAM).tar.gz...
	$(RM) $(PROGRAM).tar.gz
	tar czf $(PROGRAM).tar.gz $(OBJECT) $(SOURCE) Makefile Readme

readme: clear
	@cat Readme
	@echo

cloc: clear
	cloc --by-file $(SOURCE)



##----------------------------------------------------------------------------##
## Internal                                                                   ##
##----------------------------------------------------------------------------##

.PHONY: _build _rebuild

_build: clear
	@echo Building $(PROGRAM) - $(mode) Mode
	@echo ---------------------------------
ifneq ($(mode), $(filter $(mode), debug release))
	@echo ERROR: Invalid build mode
	@exit 1
endif

_rebuild: clear
	@echo Rebuilding $(PROGRAM) - $(mode) Mode
	@echo -----------------------------------
ifneq ($(mode), $(filter $(mode), debug release))
	@echo ERROR: Invalid build mode
	@exit 1
endif
 
