#!/bin/sh

set -e

test_string() {
  if [ "${2%x}" != "${3%x}" ]; then
    echo "$1 mismatch: expected ${2%x}, but got ${3%x}"
    exit 1
  fi
}

cleanup() {
  rm -f ../rand.bin ../zero.bin ../follow.bin
  rm -rf ../std
}

cleanup

echo "Testing basic R/W"

dd if=/dev/urandom of=../rand.bin bs=1024 count=1024 2>/dev/null
dd if=/dev/zero of=../zero.bin bs=1024 count=1024 2>/dev/null

RAND_SUM_STD=`sha1sum ../rand.bin | awk '{ print $1 }'`
ZERO_SUM_STD=`sha1sum ../zero.bin | awk '{ print $1 }'`

cp ../rand.bin ./rand.bin
cp ../zero.bin ./zero.bin

RAND_SUM=`sha1sum rand.bin | awk '{ print $1 }'`
ZERO_SUM=`sha1sum zero.bin | awk '{ print $1 }'`

test_string "rand.bin chksum" $RAND_SUM_STD $RAND_SUM
test_string "zero.bin chksum" $ZERO_SUM_STD $ZERO_SUM

echo "Testing sparse R/W"

truncate -s 4M ../rand.bin
truncate -s 4M ./rand.bin

dd if=/dev/urandom of=../follow.bin bs=1024 count=1024 2>/dev/null

cat ../follow.bin >> ../rand.bin
cat ../follow.bin >> ./rand.bin

RAND_SKIP_SUM_STD=`sha1sum ../rand.bin | awk '{ print $1 }'`
RAND_SKIP_SUM=`sha1sum rand.bin | awk '{ print $1 }'`

test_string "sparse rand.bin chksum" $RAND_SKIP_SUM_STD $RAND_SKIP_SUM

echo "Testing: Creating/Deleting dirs"
mkdir -p ../std/{a,b}/{e,f,g}/{h,i,j,k,l,m}
rm -rf ../std/a/e
FIND_OUTPUT_STD="$(find ../std -printf '%P\n' | sort)"

mkdir -p ./test/{a,b}/{e,f,g}/{h,i,j,k,l,m}
rm -rf ./test/a/e
FIND_OUTPUT="$(find ./test -printf '%P\n' | sort)"

test_string "find output" "$FIND_OUTPUT_STD" "$FIND_OUTPUT"

echo "Testing: Symlinks"
ln -s rand.bin ./rand.bin.2
ln -s test ./test_a
ln -s ../std ./test_b

RAND_SKIP_SUM_2=`sha1sum rand.bin.2 | awk '{ print $1 }'`
FIND_OUTPUT_A="$(find -L ./test_a -printf '%P\n' | sort)"
FIND_OUTPUT_B="$(find -L ./test_b -printf '%P\n' | sort)"

test_string "symlinked sparse rand.bin chksum" $RAND_SKIP_SUM_STD $RAND_SKIP_SUM_2
test_string "internal symlink find" "$FIND_OUTPUT_STD" "$FIND_OUTPUT_A"
test_string "external symlink find" "$FIND_OUTPUT_STD" "$FIND_OUTPUT_B"

echo "Testing touch won't fail"
touch touch_test

echo "Testing chmod"
chmod +x touch_test

echo "PASS, clean up"
cleanup
