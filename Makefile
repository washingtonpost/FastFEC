OUTPUT_DIR=bin
COMPILE=gcc
COMPILE_ARGS=src/urlopen.c src/memory.c src/encoding.c src/main.c -l curl -o ${OUTPUT_DIR}/fastfec

outputdir:
	mkdir -p ${OUTPUT_DIR}

build: outputdir
	${COMPILE} ${COMPILE_ARGS} 

debugbuild:
	${COMPILE} -g ${COMPILE_ARGS}

debug:
	gdb ./bin/fastfec

run:
	time ./bin/fastfec

buildrun: build run

debugrun: debugbuild debug

clean:
	rm -r ${OUTPUT_DIR}