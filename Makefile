BUILD_DIR=./cmake-build

all:
	echo ${BUILD_DIR} && if [ ! -e ${BUILD_DIR} ]; then mkdir ${BUILD_DIR}; fi && cd ${BUILD_DIR} && cmake ../ && make

test: all
	./${BUILD_DIR}/test/gtest_run

plughost: all
	cd ${BUILD_DIR} && make plughost

clean:
	cd ${BUILD_DIR} && rm -r *
	# Clean up submodule symlinks created by setup-submodules.sh
	find libs test -type l -delete 2>/dev/null || true
	rm -rf .submodule_cache

# Docker targets
docker-build-images:
	docker build -f .docker/cpp-builder.Dockerfile -t ol_dsp/cpp-builder .
	docker build -f .docker/node-builder.Dockerfile -t ol_dsp/node-builder .

docker-build-cpp:
	docker run --rm -v $(PWD):/workspace ol_dsp/cpp-builder make

docker-build-npm:
	docker run --rm -v $(PWD):/workspace ol_dsp/node-builder npm ci --ignore-scripts

docker-dev:
	docker-compose up -d dev

docker-clean:
	docker-compose down
	docker rmi ol_dsp/cpp-builder ol_dsp/node-builder ol_dsp/dev:local 2>/dev/null || true

.PHONY: all test clean docker-build-images docker-build-cpp docker-build-npm docker-dev docker-clean