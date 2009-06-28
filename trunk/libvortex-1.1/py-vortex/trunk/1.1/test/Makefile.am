all: prepare check

prepare:
	@test -h vortex.so || ln -s ../src/vortex.so vortex.so
	@cd ../src; make; cd -
check:
	python vortex-regression-client.py

