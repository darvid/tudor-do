NAME := tudor-do
CXX  := g++

BIN     := $(NAME)
OBJECTS := xkeybind.o util.o $(NAME).o

GTK_CFLAGS  := gtkmm-2.4
GTK_LDFLAGS := $(GTK_CFLAGS)

CXXFLAGS += $(foreach p,$(GTK_CFLAGS),$(shell pkg-config --cflags $(p)))

LIBS += -lX11 -lstdc++
LIBS += $(foreach p,$(GTK_LDFLAGS),$(shell pkg-config --libs $(p)))
