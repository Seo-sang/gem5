VECTOR=0
LLAMA3=0
TARGET=tokenize

CFLAGS=-static -Wall -ggdb -O0 -I/home/sanghyeon/research/gem5/include
LDFLAGS= -L/home/sanghyeon/research/gem5/util/m5/build/riscv/out -lm5
OBJDUMP=riscv64-unknown-elf-objdump

FILES = tokenize.cpp #tokenize-model-load.cpp process_model_load.cpp #

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG_ENABLE
endif

ifeq ($(LLAMA3), 1)
	FILES += llama3_process.cpp unicode-data.cpp
	CFLAGS += -DLLAMA3
else
	FILES += process.cpp
endif

ifeq ($(VECTOR), 1)
	CC=riscv64-unknown-elf-g++
	CFLAGS += -DVECTOR_ENABLE
	CFLAGS += -march=rv64gcv -mabi=lp64d
	FILES += vector_mymap.cpp vstrcmp.S
	ifeq ($(OPT), 1)
		FILES += vector_pq_static_opt.cpp
	else
		FILES += vector_pq_static.cpp
	endif
else
	CC=/opt/scalar/riscv/bin/riscv64-unknown-elf-g++
	CFLAGS += -march=rv64gc -mabi=lp64d
	FILES += mymap.cpp
	FILES += my_pq.cpp
endif

$(TARGET): $(FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	$(OBJDUMP) -D $@ > $@.dump
