#!/bin/sh

MANDOC=${MANDOC:-../mandoc}
NROFF=${NROFF:-nroff}
OUTPUT=${NROFF_OUTPUT:--Tascii}

check_skip_list() {
	[ -f skip_list ] || return 1
	while read file; do
		[ "$file" != "$1" ] || return 0
	done < skip_list 
	return 1
}

rm -rf output

echo "Starting regression tests..."
pass=0
failed=0
for file in */*.1 */*/*.1; do
	[ -f "$file" ] || continue
	check_skip_list "$file" && break
	printf "%s: " "$file"
	mandoc "$file" > test.mandoc 2> /dev/null
	${NROFF} ${OUTPUT} -mandoc "$file" > test.nroff 2> /dev/null
	if cmp -s test.mandoc test.nroff; then
		rm -f test.mandoc test.nroff
		echo "passed"
		pass=`expr $pass + 1`
	else
		file2="output/$file"
		mkdir -p `dirname $file2`
		echo "failed, see $file2"
		failed=`expr $failed + 1`
		mv test.nroff "${file2}".nroff
		mv test.mandoc "${file2}".mandoc
		diff -u "${file2}".nroff "${file2}".mandoc > "${file2}".diff
	fi
done
echo "Total: $pass passed, $failed failed"
