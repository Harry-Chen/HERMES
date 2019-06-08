#!/bin/sh
rm -rf test_file
rm -rf test_meta
mkdir -p mount
./HERMES --filedev=test_file --metadev=test_meta -f ./mount -d -o debug 2>log.txt &
ID=$!
sleep 1
mount | tail -n 3

dd if=/dev/urandom of=test_random count=1000 2>/dev/null
echo "Expected:"
sha1sum test_random
cp test_random mount/
echo "Actual:"
sha1sum mount/test_random

dd if=/dev/urandom of=test_random count=100 2>/dev/null
echo "Expected:"
sha1sum test_random
cp test_random mount/
echo "Actual:"
sha1sum mount/test_random

dd if=/dev/zero of=test_zero count=1000 2>/dev/null
echo "Expected:"
sha1sum test_zero
cp test_zero mount/
echo "Actual:"
sha1sum mount/test_zero

dd if=/dev/zero bs=4M count=16 | pv | dd of=mount/zero_file

kill $ID

ls -al mount/
