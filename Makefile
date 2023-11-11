BUILD_DIR=./cmake-build-debug
all:
	echo ${BUILD_DIR} && cd ${BUILD_DIR} && cmake ../ && make

test: all
	./${BUILD_DIR}/test/gtest_run
clean:
	cd ${BUILD_DIR} && rm -r *