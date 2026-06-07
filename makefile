MKCWD=mkdir -p $(@D)

CXX ?= g++

SANITIZERS = 			\
	-fsanitize=address 	\
	-fsanitize=undefined


CFLAGS_WARNS ?= 	\
		-Werror 	\
		-Wextra 	\
		-Wall 		\
		-Wundef 	\
		-Wshadow 	\
		-Wvla

CXXFLAGS = 			\
		-O2 		\
		-g 		 	\
		-std=c++2b 	\
		-Isrc/      \
		-DIMGUI_DEFINE_MATH_OPERATORS=1\
		-Iexternal/imgui \
		-Iexternal/imgui/backends \
		$(shell pkgconf --cflags sdl3) \
		$(CFLAGS_WARNS)

LDFLAGS=$(SANITIZERS)

# some people likes to use sources/source instead of src
PROJECT_NAME = spinetail
BUILD_DIR = build
# you may want to update compile_flags.txt after changing this value
SRC_DIR = src
EXT_DIR = external
IMGUI_CORE = \
	$(EXT_DIR)/imgui/imgui.cpp \
	$(EXT_DIR)/imgui/imgui_draw.cpp \
	$(EXT_DIR)/imgui/imgui_tables.cpp \
	$(EXT_DIR)/imgui/imgui_widgets.cpp

IMGUI_BACKENDS = \
	$(EXT_DIR)/imgui/backends/imgui_impl_sdl3.cpp \
	$(EXT_DIR)/imgui/backends/imgui_impl_sdlgpu3.cpp

# avoid using '**' because in some cases it may not work
CXXFILES = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp) $(wildcard $(SRC_DIR)/*/*/*.cpp) $(IMGUI_CORE) $(IMGUI_BACKENDS)

DFILES = $(patsubst %.cpp, $(BUILD_DIR)/%.d, $(CXXFILES))
OFILES = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(CXXFILES))

OUTPUT = build/$(PROJECT_NAME)


$(OUTPUT): $(OFILES)
	@$(MKCWD)
	@echo " LD  [ $@ ] $<"
	@$(CXX) -o $@ $^ $(LDFLAGS) $(shell pkgconf --libs sdl3)

$(BUILD_DIR)/%.o: %.cpp
	@$(MKCWD)
	@echo " CXX [ $@ ] $<"
	@$(CXX) $(CXXFLAGS) -MMD -MP $< -c -o $@

run: $(OUTPUT)
	@$(OUTPUT) ./tests/test2.tail

all: $(OUTPUT)

clean:
	@rm -rf build/

.PHONY: clean all run

-include $(DFILES)
