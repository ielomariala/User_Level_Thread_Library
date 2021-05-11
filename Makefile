CFLAGS = -Wall -Wextra -fPIC
BINPATH = ./install/bin
LIBPATH = ./install/lib
PREEMPT_FOLDER = ./preemption
LIBPREEMPTPATH = ./install/lib/preempt
GRAPHSPATH = ./graphs
PTHREAD_BIN_PATH = ./pthread_bin
PTHREADFLAG = -DUSE_PTHREAD -lpthread
VLG = --leak-check=full --show-reachable=yes --track-origins=yes
LDLIBS = -lthread -L.
LIBINCLUDE = LD_LIBRARY_PATH=$(LIBPATH)
LIBINCLUDEPREEMPT = LD_LIBRARY_PATH=$(LIBPATH)/preempt
DEBUG_FLAG = -DDEBUG
DEBUG_COMPIL = -fPIC -g -O0

BIN = 01-main \
	 02-switch \
	 03-equity \
	 11-join \
	 12-join-main \
	 21-create-many \
	 22-create-many-recursive \
	 23-create-many-once \
	 31-switch-many \
	 32-switch-many-join \
	 33-switch-many-cascade \
	 51-fibonacci \
	 61-mutex \
	 62-mutex \
	 71-preemption \
	 81-deadlock \
	 91-divide-conquer-search \
	 92-divide-conquer-sum \
	 93-rapid-sort  \
	 94-fusion-sort \
	 101-stack-overflow \
	 102-large-var-stack-overflow \
	 111-signaux

LIB = libthread.so
LIBPREEMPT = libthread_preempt.so


.PHONY: all graphs
all: $(LIB) $(BIN)

libthread.so: thread.c thread.h signals.c signals.h
	gcc $(CFLAGS) -g thread.c signals.c -shared -lpthread -o $@

lib-no-deadlock: CFLAGS += -DNO_DEADLOCK
lib-no-deadlock: $(LIB)

lib-no-preemption: CFLAGS += -DNO_PREEMPTION
lib-no-preemption: $(LIB)

lib-no-mprotect: CFLAGS += -DNO_MPROTECT
lib-no-mprotect: $(LIB)

performance: CFLAGS += -DNO_DEADLOCK -DNO_PREEMPTION -DNO_MPROTECT
performance: all

libthread_preempt.so: $(PREEMPT_FOLDER)/thread.c thread.h
	gcc $(CFLAGS) $(PREEMPT_FOLDER)/thread.c -shared -lpthread -o $(LIBPREEMPTPATH)/libthread.so

debug: CFLAGS += -DDEBUG -g -O0
debug: all

check:
	$(LIBINCLUDE) $(BINPATH)/01-main
	$(LIBINCLUDE) $(BINPATH)/02-switch
	$(LIBINCLUDE) $(BINPATH)/03-equity
	$(LIBINCLUDE) $(BINPATH)/11-join
	$(LIBINCLUDE) $(BINPATH)/12-join-main
	$(LIBINCLUDE) $(BINPATH)/21-create-many 8
	$(LIBINCLUDE) $(BINPATH)/22-create-many-recursive 8
	$(LIBINCLUDE) $(BINPATH)/23-create-many-once 8
	$(LIBINCLUDE) $(BINPATH)/31-switch-many 4 5
	$(LIBINCLUDE) $(BINPATH)/32-switch-many-join 4 5
	$(LIBINCLUDE) $(BINPATH)/33-switch-many-cascade 4 5
	$(LIBINCLUDE) $(BINPATH)/51-fibonacci 16
	$(LIBINCLUDE) $(BINPATH)/61-mutex 5
	$(LIBINCLUDE) $(BINPATH)/62-mutex 5
	$(LIBINCLUDE) $(BINPATH)/71-preemption 5
	$(LIBINCLUDE) $(BINPATH)/91-divide-conquer-search 8 6 2
	$(LIBINCLUDE) $(BINPATH)/92-divide-conquer-sum 8 6 2
	$(LIBINCLUDE) $(BINPATH)/93-rapid-sort 8 6 2
	$(LIBINCLUDE) $(BINPATH)/94-fusion-sort 8 6 2
	$(LIBINCLUDE) $(BINPATH)/111-signaux
	$(LIBINCLUDE) $(BINPATH)/81-deadlock
	$(LIBINCLUDE) $(BINPATH)/101-stack-overflow
	$(LIBINCLUDE) $(BINPATH)/102-large-var-stack-overflow
	

valgrind: 
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/01-main 
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/02-switch
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/03-equity
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/11-join
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/12-join-main
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/21-create-many 8
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/22-create-many-recursive 20
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/23-create-many-once 8
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/31-switch-many 4 5
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/32-switch-many-join 4 5
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/33-switch-many-cascade 4 5
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/51-fibonacci 8
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/61-mutex 5
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/62-mutex 5 
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/71-preemption 5
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/91-divide-conquer-search 7 6 9 0 9 10 7 8
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/92-divide-conquer-sum 4 3 3 23 14 26
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/93-rapid-sort 19 6 8 7 62
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/94-fusion-sort 13 6 8 7 62 43
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/111-signaux
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/81-deadlock
	$(LIBINCLUDE) valgrind $(VLG) $(BINPATH)/101-stack-overflow

	

test: 
	$(LIBINCLUDE) $(BINPATH)/111-signaux



pthreads: CFLAGS += $(PTHREADFLAG)
pthreads: LDLIBS = -lpthread
pthreads: $(BIN)
pthreads: 
	cp $(BIN) $(PTHREAD_BIN_PATH)

graphs: graphs.py
	python3 graphs.py

install: $(LIB) $(BIN)
	mv $(BIN) $(BINPATH)
	mv $(LIB) $(LIBPATH)
	
.PHONY: clean clean_all

clean:
	rm -rf *.o $(BIN) $(LIB) $(BINPATH)/*
	rm -rf $(LIBPREEMPTPATH)/*
	rm -f $(LIBPATH)/lib*

clean_all:
	rm -rf *.o $(BIN) $(LIB) $(BINPATH)/* $(GRAPHSPATH)/* $(PTHREAD_BIN_PATH)/*
	rm -rf $(LIBPREEMPTPATH)/*
	rm -f $(LIBPATH)/lib*

