all:
	cd src && make

clean:
	cd src && make clean

tests:
	./run_tests.sh
