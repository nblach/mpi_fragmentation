#!/bin/bash
source conf.sh

GREEN=$(tput setaf 2)
RED=$(tput setaf 1)
NC=$(tput sgr0)

${MPI_COMPILER} -c -fPIC libstripe/libstripe.c -o libstripe/libstripe.o
if [ ! -f "libstripe/libstripe.o" ]; then
    echo "${RED}[Error] libstripe.o compilation failed, please check error messages above.${NC}"
    exit 1
fi

${MPI_COMPILER} -c -fPIC thread-pool/thpool.c -o thread-pool/thpool.o
if [ ! -f "thread-pool/thpool.o" ]; then
    echo "${RED}[Error] thpool.o compilation failed, please check error messages above.${NC}"
    exit 1
fi

${MPI_COMPILER} -shared -pthread -o libstripe/libstripe.so libstripe/libstripe.o thread-pool/thpool.o
if [ ! -f "libstripe/libstripe.so" ]; then
    echo "${RED}[Error] libstripe.so compilation failed, please check error messages above.${NC}"
    exit 1
fi

echo "${GREEN}Everything compiled successfully.${NC}"
