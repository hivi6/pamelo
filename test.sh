#!/bin/bash

flags=("" "--print-token" "--print-ast" "--print-ir")

exit_code=0
for flag in "${flags[@]}"; do
	ext=`echo $flag | cut -d '-' -f 4 | xargs`
	if [ "$ext" == "" ]; then
		ext="out"
	fi

	echo "Test command: ./build/pamelo $flag <filename> 2>&1 | diff - <filename>.$ext"
	all_passed=1
	for test in `find tests -name '*.pam' | sort`; do
		./build/pamelo $flag $test 2>&1 | diff - $test.$ext &> /dev/null
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
		exit_code=1
	fi
	echo
done

exit $exit_code

