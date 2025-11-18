# Makefile for ERP project
# Usage:
#   make all                # build everything (no-OS-threads fallback)
#   make all THREAD=std     # build using C++ std::thread
#   make all THREAD=pthread # build using POSIX pthreads (add -pthread)
#   make run-menu           # run the interactive menu executable
#   make run-q1             # run program for Q1
#   make clean              # remove binaries and objects
#
# Note: your directory should contain:
# basicIO.cpp basicIO.h erp_menu.cpp erp_q1.cpp erp_q2.cpp erp_q3.cpp erp_q4.cpp erp_Q5.cpp mythread_noos.h students_3000.csv

CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
LDFLAGS :=

# THREAD selection: none (default -> uses mythread_noos.h),
# std  -> uses -DUSE_STD_THREAD
# pthread -> uses -DUSE_POSIX and links -pthread
THREAD ?= none

ifeq ($(THREAD),std)
	THREAD_DEFS := -DUSE_STD_THREAD
endif
ifeq ($(THREAD),pthread)
	THREAD_DEFS := -DUSE_POSIX
	LDFLAGS += -pthread
endif

# binaries (lowercase names)
BINS := erp_menu erp_q1 erp_q2 erp_q3 erp_q4 erp_q5

# Map each binary to its corresponding source file (note: erp_Q5.cpp in your directory)
erp_menu_SRC := erp_menu.cpp
erp_q1_SRC  := erp_q1.cpp
erp_q2_SRC  := erp_q2.cpp
erp_q3_SRC  := erp_q3.cpp
erp_q4_SRC  := erp_q4.cpp
erp_q5_SRC  := erp_Q5.cpp

# common dependencies
COMMON_HDR := basicIO.h mythread_noos.h
COMMON_OBJS := basicIO.o

.PHONY: all build clean help run-menu run-q1 run-q2 run-q3 run-q4 run-q5

all: build

build: $(BINS)
	@echo "Build complete. (THREAD=$(THREAD))"

# build each binary from its source
erp_menu: $(erp_menu_SRC) $(COMMON_OBJS) mythread_noos.h
	$(CXX) $(CXXFLAGS) $(THREAD_DEFS) $< $(COMMON_OBJS) -o $@ $(LDFLAGS)

erp_q1: $(erp_q1_SRC) $(COMMON_OBJS) mythread_noos.h
	$(CXX) $(CXXFLAGS) $(THREAD_DEFS) $< $(COMMON_OBJS) -o $@ $(LDFLAGS)

erp_q2: $(erp_q2_SRC) $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(THREAD_DEFS) $< $(COMMON_OBJS) -o $@ $(LDFLAGS)

erp_q3: $(erp_q3_SRC) $(COMMON_OBJS) mythread_noos.h
	$(CXX) $(CXXFLAGS) $(THREAD_DEFS) $< $(COMMON_OBJS) -o $@ $(LDFLAGS)

erp_q4: $(erp_q4_SRC) $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(THREAD_DEFS) $< $(COMMON_OBJS) -o $@ $(LDFLAGS)

erp_q5: $(erp_q5_SRC) $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(THREAD_DEFS) $< $(COMMON_OBJS) -o $@ $(LDFLAGS)

# compile basicIO.o (if present)
basicIO.o: basicIO.cpp basicIO.h
	$(CXX) $(CXXFLAGS) -c basicIO.cpp -o basicIO.o

# Convenience run targets
run-menu: erp_menu
	@echo "Running erp_menu..."
	./erp_menu

run-q1: erp_q1
	@echo "Running erp_q1..."
	./erp_q1

run-q2: erp_q2
	@echo "Running erp_q2..."
	./erp_q2

run-q3: erp_q3
	@echo "Running erp_q3..."
	./erp_q3

run-q4: erp_q4
	@echo "Running erp_q4..."
	./erp_q4

run-q5: erp_q5
	@echo "Running erp_q5..."
	./erp_q5

# small helper to run all tests sequentially (prints headings)
run-all: erp_q1 erp_q2 erp_q3 erp_q4 erp_q5
	@echo "====== Running Q1 ======"
	./erp_q1 || true
	@echo "====== Running Q2 ======"
	./erp_q2 || true
	@echo "====== Running Q3 ======"
	./erp_q3 || true
	@echo "====== Running Q4 ======"
	./erp_q4 || true
	@echo "====== Running Q5 ======"
	./erp_q5 || true

clean:
	@echo "Cleaning binaries and object files..."
	-rm -f $(BINS) *.o students_sorted.csv students_sorted_q3.csv students_sorted_menu.csv
	@echo "Clean done."

help:
	@echo "Makefile targets:"
	@echo "  make all             -> build all binaries (default THREAD=none)"
	@echo "  make all THREAD=std  -> build using C++ std::thread"
	@echo "  make all THREAD=pthread -> build using POSIX pthreads (links -pthread)"
	@echo "  make run-menu        -> run the interactive menu (erp_menu)"
	@echo "  make run-q1 ... run-q5 -> run corresponding question binary"
	@echo "  make run-all         -> run q1..q5 sequentially"
	@echo "  make clean           -> remove binaries and object files"

# implicit rule fallback: if user added sources not covered above, pattern rule
%: %.cpp $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(THREAD_DEFS) $< $(COMMON_OBJS) -o $@ $(LDFLAGS)
