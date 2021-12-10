all:
	cd Pigeon && $(MAKE) -j8 && cd ../Test1 && $(MAKE)
