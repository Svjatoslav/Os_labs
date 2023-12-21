#include <iostream>
#include <fstream>
#include <filesystem>
#include <sys/syslog.h>

#include "test.h"

int cmp(const int& a, const int& b) {
    return a - b;
}

void testing(int readersCnt = 2, int writersCnt = 2, int arrSize = 1000, int testsCnt = 5) {
    Tester::writeTesting(writersCnt, arrSize, testsCnt);
    Tester::readTesting(readersCnt, arrSize, testsCnt);
    Tester::writeAndReadTesting( readersCnt, writersCnt, arrSize, testsCnt);
}

int main(int argc, char* argv[]) {
    openlog("Lab3 log", LOG_NDELAY | LOG_PID, LOG_USER);
    int size;
    int readersCnt, writersCnt;
    int testCnt;
    if (argc!= 2){
        std::cout<<"Config path wasn't entered."<<std::endl;
        return EXIT_FAILURE;
    }
    std::ifstream file(argv[1]);
    if(file.is_open()){ 
        if(file >> readersCnt >> writersCnt >> size >> testCnt){
            testing(readersCnt, writersCnt, size, testCnt);
        }
        else {
            std::cout<<"Config file inccorect."<<std::endl;
            return EXIT_FAILURE;
        } 
    } else {
        std::cout<<"Config file open error."<<std::endl;
        return EXIT_FAILURE;
    }
    return 0;
    
}
