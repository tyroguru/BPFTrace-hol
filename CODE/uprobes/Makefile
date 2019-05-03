all:	uprobeme

uprobeme:	uprobeme.cc warning
		clang++ -g -o uprobeme uprobeme.cc -static

.PHONY: warning
warning:
	# echo "If you get compile errors you might need to install the glibc-static package"

clean:
	rm uprobeme
