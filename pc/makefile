.PHONY: all

all:
	@g++ ./src/configurator.cpp -Wall -O2 -o ./bin/configurator
	@g++ ./src/reader.cpp -Wall -O2 -o ./bin/reader -lsqlite3 -lcurl
	@g++ ./src/shower.cpp -Wall -O2 -o ./bin/shower -l sqlite3 

clean:
	@rm -f ./bin/configurator
	@rm -f ./bin/reader
	@rm -f ./bin/shower

