.PHONY: build test

build:
	../tools/png2cpp.py \
		--input ../png/shaposans32_fh.png \
		--output_cpp src/shaposans32_fh.cpp \
		--output_hpp include/mamefont/shaposans32_fh.hpp \
		--cpp_namespace mfnt \
		--include_dir mamefont

test:
	mkdir -p build
	g++ \
		-Iinclude \
		test/test.cpp \
		src/shaposans32_fh.cpp \
		-o build/test
	./build/test

