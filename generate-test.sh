#!/bin/bash

for test in `find tests -name '*.pam' | sort`; do 
	./build/pamelo $test &> $test.out;
	./build/pamelo --print-ir $test &> $test.ir; 
	./build/pamelo --print-ast $test &> $test.ast;
	./build/pamelo --print-token $test &> $test.token;
done 

