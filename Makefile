obj-m += avx512emulation.o

all: app
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

app: app.c
	gcc $< -mavx512f -O0 -g -o $@
