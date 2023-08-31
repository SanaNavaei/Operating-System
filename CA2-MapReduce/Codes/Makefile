all: GenreCounter map reduce

GenreCounter: GenreCounter.cpp
	g++ -g -std=c++17 GenreCounter.cpp -o GenreCounter.out

map: map.cpp
	g++ -g -std=c++17 map.cpp -o map.out

reduce: reduce.cpp
	g++ -g -std=c++17 reduce.cpp -o reduce.out

.PHONY: clean

clean:
	rm GenreCounter.out map.out reduce.out