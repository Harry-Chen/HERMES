#!/bin/bash

MOUNT_DIR=mount

mount_hermes() {
	MOUNT_DIR=$2
	echo "Mounting HERMES on ${MOUNT_DIR}"
	rm -rf ${MOUNT_DIR}
	mkdir -p ${MOUNT_DIR}
	
	TEST_PATH="/dev/shm/HERMES_test"
	rm -rf ${TEST_PATH}
	mkdir -p ${TEST_PATH}
	
	HERMES=$1
	${HERMES} -f --metadev=${TEST_PATH}/meta --filedev=${TEST_PATH}/file ${MOUNT_DIR} &
	HERMES_PID=$!
}

rm -rf ${MOUNT_DIR}
mkdir -p ${MOUNT_DIR}

for BACKEND in 'EXT4' 'LevelDB' 'RocksDB' 'BerkeleyDB' 'Vedis'
do
	if [ ${BACKEND} != 'EXT4' ] ;then
		rm -rf backend/${BACKEND}
		mkdir -p backend/${BACKEND}
		cd backend/${BACKEND}
		env CXX=clang++ cmake ../../../ -DBACKEND=${BACKEND}
		make -j16
		cd ../../
		HERMES=backend/${BACKEND}/HERMES
		echo "Use executable: ${HERMES}"
	fi

    if [ ${BACKEND} != 'EXT4' ] ;then
        mount_hermes ${HERMES} ${MOUNT_DIR}
        # no hurry...
        sleep 3
    fi

    cd ${MOUNT_DIR}

    echo "Testing: ${BACKEND}"
    ../test_correctness_single.sh

    cd ..
    
    if [ ${BACKEND} != 'EXT4' ] ;then
        kill -s SIGTERM ${HERMES_PID}
    fi
    
    sleep 3
done

