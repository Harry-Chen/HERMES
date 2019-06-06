#!/bin/bash

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

mkdir -p results

c++ -o fast_create fast_create.cpp

for BACKEND in 'EXT4' 'LevelDB' 'RocksDB' 'BerkeleyDB'
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

	MOUNT_DIR=mount

	for FILES in '1000' '10000' '100000' '1000000'
	do
		if [ ${BACKEND} != 'EXT4' ] ;then
			mount_hermes ${HERMES} ${MOUNT_DIR}
            sleep 3
		fi

		# run benchmark and save results
		cd ${MOUNT_DIR}
		RESULT_FILE=../results/${BACKEND}_create-file_${FILES}.txt
        echo "Create files cost:" > ${RESULT_FILE}
        { time ../fast_create $FILES; } 2>> ${RESULT_FILE}
		echo "Enumerate files cost:" >> ${RESULT_FILE}
		{ time ls -f; } > /dev/null 2>> ${RESULT_FILE}
		echo "List files with attr cost:" >> ${RESULT_FILE}
		{ time ls -lf; } > /dev/null 2>> ${RESULT_FILE}

		cd ..

		if [ ${BACKEND} != 'EXT4' ] ;then
			kill -s SIGTERM ${HERMES_PID}
		fi
		sleep 3
	done
done

