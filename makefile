thread: vmm.c
	g++ vmm.c -lm
all: run
run: vmm
	./vmm