all:	closeall tempfiles mapit

closeall:
		clang++ -o closeall closeall.cc -static

tempfiles:
		clang++ -o tempfiles tempfiles.cc -static

mapit:	mapit.cc
		clang++ -g -o mapit mapit.cc -static

clean:
	rm closeall tempfiles mapit
