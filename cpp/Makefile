.PHONY: build test

build:
	../tools/png2cpp.py \
		--input ../png/shaposans32_fh.png \
		--output_cpp mamefont/src/shaposans32_fh.cpp \
		--output_hpp mamefont/include/mamefont/shaposans32_fh.hpp \
		--cpp_namespace mfnt \
		--include_dir mamefont

test:
	mkdir -p build
	g++ \
		-Imamefont/include \
		test/test.cpp \
		mamefont/src/shaposans32_fh.cpp \
		-o build/test
	./build/test

