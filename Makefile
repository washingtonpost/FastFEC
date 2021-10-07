OUTPUT_DIR=bin
COMPILE=gcc -Wall
INCLUDE_FILES=src/memory.c src/urlopen.c src/encoding.c src/csv.c src/writer.c src/fec.c src/buffer.c -l curl -l pcre
COMPILE_ARGS=${INCLUDE_FILES} src/main.c -o ${OUTPUT_DIR}/fastfec
COMPILE_LIB_ARGS=${INCLUDE_FILES} -shared -o ${OUTPUT_DIR}/fastfec.so

outputdir:
	mkdir -p ${OUTPUT_DIR}

build: outputdir
	${COMPILE} -Ofast ${COMPILE_ARGS}

buildlib: outputdir
	${COMPILE} -Ofast ${COMPILE_LIB_ARGS}

debugbuild:
	${COMPILE} -g ${COMPILE_ARGS}

debug:
	gdb ./bin/fastfec

run:
	time ./bin/fastfec

test: src/*_test.c
	@for test in $^; do echo "$${test}"; [ -f "$${test}" ] || break; ${COMPILE} ${INCLUDE_FILES} $${test} -o ${OUTPUT_DIR}/test; ./${OUTPUT_DIR}/test; done;

buildrun: build run

debugrun: debugbuild debug

clean:
	rm -r ${OUTPUT_DIR}