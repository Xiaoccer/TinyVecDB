BUILD_DIR = ${CURDIR}/build/

PROTOC = ${CURDIR}/../third_party/installed/bin/protoc

SOURCES = $(shell find ${CURDIR} -name "*.proto")
OBJECTS = $(patsubst ${CURDIR}/%.proto, ${BUILD_DIR}/gen_cpp/%.pb.cc, ${SOURCES})
HEADERS = $(patsubst ${CURDIR}/%.proto, ${BUILD_DIR}/gen_cpp/%.pb.h, ${SOURCES})

all: ${OBJECTS} ${HEADERS}
.PHONY: all

${BUILD_DIR}/gen_cpp/%.pb.h ${BUILD_DIR}/gen_cpp/%.pb.cc: ${CURDIR}/%.proto | ${BUILD_DIR}/gen_cpp
	${PROTOC} --proto_path=${CURDIR} --cpp_out=${BUILD_DIR}/gen_cpp $<

${BUILD_DIR}/gen_cpp:
	mkdir -p $@
