#!/bin/sh
dd if=/dev/urandom of=test_random count=1000 2>/dev/null
echo "Expected:"
sha1sum test_random
mkdir -p mount
./HERMES --filedev=test_file --metadev=meta_dev -f ./mount &
ID=$!
cp test_random mount/
echo "Actual:"
sha1sum mount/test_random
kill $ID
