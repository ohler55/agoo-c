
all:
	make -C src

clean:
	make -C src clean
	rm -rf include lib

release:
	make clean
	make build=release

dev:
	make clean
	make
