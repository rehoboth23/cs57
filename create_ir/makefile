LLVMCODE = llvm_builder
TEST = test

$(LLVMCODE): $(LLVMCODE).c
	clang -g `llvm-config --cflags` -I /usr/include/llvm-c/ -c $(LLVMCODE).c
	clang++ `llvm-config --cxxflags --ldflags --libs core` -I /usr/include/llvm-c/ $(LLVMCODE).o -o $@

minic:
	./$(LLVMCODE)
	clang main.c $(TEST).ll -o $(TEST).out

llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).s

clean: 
	rm -rf $(TEST).ll
	rm -rf $(LLVMCODE)
	rm -rf *.o
	rm -rf *.out
