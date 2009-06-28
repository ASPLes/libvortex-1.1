all: build_extension

build_extension:
	python setup.py build_ext --include-dirs /usr/include/vortex-1.1:/usr/include/axl:. --inplace

clean: 
	rm -rf build/
	rm -f vortex.so
