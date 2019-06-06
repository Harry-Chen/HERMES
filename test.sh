#!/bin/sh
rm test_file
rm test_meta
mkdir -p mount
./HERMES --filedev=test_file --metadev=test_meta -f ./mount &
ID=$!

dd if=/dev/urandom of=test_random count=1000 2>/dev/null
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

kill $ID
