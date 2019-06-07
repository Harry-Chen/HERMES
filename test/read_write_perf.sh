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

	MOUNT_DIR=mount

	for FIO_TEST in 'seq-read' 'seq-write' 'rand-read' 'rand-write'
	do
		for BS in '1k' '4k' '16k'
		do
			if [ ${BACKEND} != 'EXT4' ] ;then
				mount_hermes ${HERMES} ${MOUNT_DIR}
			fi
			
			# run benchmark and save results
			cd ${MOUNT_DIR}
			env BS=${BS} fio --group_reporting ../${FIO_TEST}.fio | tee ../results/${BACKEND}_${FIO_TEST}_${BS}.txt
			cd ..
			
			if [ ${BACKEND} != 'EXT4' ] ;then
				kill -s SIGTERM ${HERMES_PID}
			fi
			
			sleep 3
		done
	done
done

