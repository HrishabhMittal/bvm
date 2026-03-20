all:
	g++ src/main.cpp -o main -O3 -s \
    	-ffunction-sections -fdata-sections \
    	-Wl,--gc-sections \
    	--static
