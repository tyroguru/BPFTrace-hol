all:	closeall tempfiles mapit

closeall:
		clang++ -o closeall closeall.cc

tempfiles:
		clang++ -o tempfiles tempfiles.cc

mapit:	mapit.cc
		clang++ -g -o mapit mapit.cc

clean:
	rm closeall tempfiles mapit
