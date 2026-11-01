CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -I.

TARGET = test_runner

SOURCES = test_runner.cpp \
          platform/Tracer.cpp \
          1_monolithic/MonolithicKernel.cpp \
          1_monolithic/MonolithicOSApi.cpp \
          2_layered/Layer0_Hardware.cpp \
          2_layered/Layer1_Drivers.cpp \
          2_layered/Layer2_FileSystem.cpp \
          2_layered/Layer3_OSApi.cpp \
          3_modular/StandardModules.cpp \
          3_modular/ModularKernel.cpp \
          3_modular/ModularOSApi.cpp

all: $(TARGET)

HEADERS = $(wildcard platform/*.hpp app/*.hpp 1_monolithic/*.hpp 2_layered/*.hpp 3_modular/*.hpp) test_support.hpp

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
