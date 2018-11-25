
all:
	make -C src refresh
	make -C src
	make -C test

clean:
	make -C src clean
	make -C test clean
	rm -rf include lib

test: all
	make -C test test

agoo: all
	make -C src agoo


pull:
	cd ../agoo && git pull
	git pull

release:
	make clean
	make src refresh
	make build=release

dev:
	make clean
	make agoo
	make
