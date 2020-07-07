CXX=g++-9 -std=c++11

OpenVisus_DIR=/Users/scrgiorgio/Desktop/OpenVisus/build_gcc/Release/OpenVisus

CXX_FLAGS=\
	-I$(OpenVisus_DIR)/include/Kernel \
	-I$(OpenVisus_DIR)/include/Db \
	-DVISUS_STATIC_KERNEL_LIB=1 \
	-DVISUS_STATIC_DB_LIB=1

main: main.o
	$(CXX) -o $@ $< -L$(OpenVisus_DIR)/lib -lVisusIO
 
main.o: main.cpp 
	$(CXX) $(CXX_FLAGS) -c -o $@ $< 


clean:
	rm -f  main main.o

.PHONY: clean
