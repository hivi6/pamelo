#!/bin/bash

all_passed=1

for test in `find tests -name '*.pam' | sort`; do
	./build/pamelo $test 2>&1 | diff - $test.out &>/dev/null
	passed=$?

	if [ $passed == 0 ]; then
		echo PASSED - $test
	else
		echo FAILED - $test
		all_passed=0
	fi
done

if [ $all_passed == 1 ]; then
	echo PASSED
else
	echo FAILED
fi

