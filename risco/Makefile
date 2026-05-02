# ================================================================
# Makefile — Processador RISCO em SystemC
# ================================================================
# Pré-requisito: SystemC instalado
# Ubuntu/Debian: sudo apt install libsystemc-dev
# ================================================================

SYSTEMC_HOME ?= /usr/local/systemc-2.3.3
CXX          = g++
CXXFLAGS     = -std=c++17 -Wall -O2
INCLUDES     = -I$(SYSTEMC_HOME)/include -I.
LDFLAGS      = -L$(SYSTEMC_HOME)/lib-linux64 -lsystemc -lm

# Tenta detectar instalação via apt (libsystemc-dev)
SYSTEMC_APT  = /usr/include/systemc
ifneq ($(wildcard $(SYSTEMC_APT)),)
  INCLUDES   = -I/usr/include -I.
  LDFLAGS    = -lsystemc -lm
endif

TARGET = risco_sim
SRCS   = testbench.cpp
HDRS   = ula.h banco_reg.h memoria.h controle.h datapath.h

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SRCS) $(LDFLAGS)
	@echo ""
	@echo "Compilado com sucesso! Execute: ./$(TARGET)"

run: $(TARGET)
	./$(TARGET)

wave: run
	gtkwave risco_trace.vcd &

clean:
	rm -f $(TARGET) *.vcd *.o

.PHONY: all run wave clean
