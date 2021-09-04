OUTPUT_DIR=bin
COMPILE=gcc

build:
	mkdir -p ${OUTPUT_DIR}
	${COMPILE} src/urlopen.c src/main.c -l curl -o ${OUTPUT_DIR}/fastfec

run:
	time ./bin/fastfec

buildrun: build run

clean:
	rm -r ${OUTPUT_DIR}