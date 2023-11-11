BUILD_DIR=./cmake-build
all:
	echo ${BUILD_DIR} && if [ ! -e ${BUILD_DIR} ]; then mkdir ${BUILD_DIR}; fi && cd ${BUILD_DIR} && cmake ../ && make

test: all
	./${BUILD_DIR}/test/gtest_run
clean:
	cd ${BUILD_DIR} && rm -r *