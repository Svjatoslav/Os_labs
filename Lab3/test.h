#pragma once

#include <iostream>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <unistd.h>
#include "opt_set.h"


int getRand(int minNum, int maxNum) {
    return rand()% (maxNum - minNum + 1) + minNum;
}

std::vector<int> generateItems(int size) {
    constexpr int MIN = 0;
    const int MAX = 3 * size;
    std::vector<int> res;
    res.reserve(size);
    std::srand(std::time(0));
    for (int i = 0; i < size; ++i)
        res.push_back(getRand(MIN, MAX));
    return res;
}

struct writeArgs_t {
    OptSet<int>* set;
    int begin;
    int end;
    const std::vector<int>* items;
    writeArgs_t(OptSet<int>* set, int begin, int end, const std::vector<int>* items) :
      set(set), begin(begin), end(end), items(items) {}
};

struct readArgs_t {
    OptSet<int>* set;
    int threadNum;
    int step;
    std::vector<int>* checkArray;
    readArgs_t(OptSet<int>* set, int threadNum, int step, std::vector<int>* checkArray) :
      set(set), threadNum(threadNum), step(step), checkArray(checkArray) {}
};

class Tester {
private:
    static pthread_cond_t cv;
    static bool start;
    
    static void synchronize() {
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_lock(&mutex);
        if (!start)
            pthread_cond_wait(&cv, &mutex);
        else
            pthread_cond_broadcast(&cv);
        pthread_mutex_unlock(&mutex);
    }

    static void startTest() {
        start = true;
        pthread_cond_broadcast(&cv);
    }

    static void* threadWrite(void* inputArgs) {
        writeArgs_t* args = reinterpret_cast<writeArgs_t*>(inputArgs);
        synchronize();
        for (int i = args->begin; i < args->end; ++i)
            args->set->add((*(args->items))[i]);
        return (void*)nullptr;
    }

    static void* threadRead(void* inputArgs) {
        readArgs_t* args = static_cast<readArgs_t*>(inputArgs);
        synchronize();
        int size = int(args->checkArray->size());
        for (int i = 0; i * args->step + args->threadNum < size; ++i) {
            if (args->set->contains(i * args->step + args->threadNum)) {
                (*args->checkArray)[i * args->step + args->threadNum] = 1;
                args->set->remove(i * args->step + args->threadNum);
            }
        }
        return (void*)nullptr;
    }

    static void prepareWriters(OptSet<int>& set, const std::vector<int>& items,
        std::vector<pthread_t>& threads, std::vector<writeArgs_t>& args) {
        int n_writers = (int)threads.size();
        args.reserve(n_writers);
        int begin = 0;
        int size = items.size();
        int step = size / n_writers;
        int end = step;
        start = false;
        for (int i = 0; i < n_writers; ++i) {
            if (i == n_writers - 1)
                end = size;
            args.emplace_back(&set, begin, end, &items);
            pthread_create(&threads[i], nullptr, threadWrite, (void*)&args.back());
            begin += step;
            end += step;
            }
        }

    static void prepareReaders(OptSet<int>& set, std::vector<int>& checkArr, std::vector<pthread_t>& threads, std::vector<readArgs_t>& args) {
        int n_readers = (int)threads.size();
        args.reserve(n_readers);
        start = false;
        for (int i = 0; i < n_readers; ++i) {
            args.emplace_back(&set, i, n_readers, &checkArr);
            pthread_create(&threads[i], nullptr, threadRead, (void*)&args.back());
        }
    }
    
    static long long writerTest(int n_writers, OptSet<int>& set, const std::vector<int>& items) {
        std::vector<pthread_t> threads(n_writers);
        std::vector<writeArgs_t> args;
        prepareWriters(set, items, threads, args);
        auto t1 = std::chrono::high_resolution_clock::now();
        startTest();
        for (int i = 0; i < n_writers; ++i)
            pthread_join(threads[i], nullptr);
        auto t2 = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    }

    static long long readerTest(int n_readers, OptSet<int>& set, std::vector<int>& checkArr) {
    n_readers = n_readers > sysconf(_SC_NPROCESSORS_ONLN) ? sysconf(_SC_NPROCESSORS_ONLN) : n_readers;
        std::vector<pthread_t> threads(n_readers);
        std::vector<readArgs_t> args;
        prepareReaders(set, checkArr, threads, args);
        auto t1 = std::chrono::high_resolution_clock::now();
        startTest();
        for (int i = 0; i < n_readers; ++i)
            pthread_join(threads[i], nullptr);
        auto t2 = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    }

    static long long generalTest(int n_readers, int n_writers, OptSet<int>& set,
    const std::vector<int>& items, std::vector<int>& checkArr) {
        std::vector<pthread_t> readers(n_readers);
        std::vector<readArgs_t> readersArgs;
        std::vector<pthread_t> writers(n_writers);
        std::vector<writeArgs_t> writersArgs;
        prepareReaders(set, checkArr, readers, readersArgs);
        prepareWriters(set, items, writers, writersArgs);
        auto t1 = std::chrono::high_resolution_clock::now();
        startTest();
        for (int i = 0; i < n_readers; ++i)
            pthread_join(readers[i], nullptr);
        for (int i = 0; i < n_writers; ++i)
            pthread_join(writers[i], nullptr);
        auto t2 = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    }

public:  
    static void writeTesting(int n_writers, int size, int numOfTests) {
        n_writers = n_writers > sysconf(_SC_NPROCESSORS_ONLN) ? sysconf(_SC_NPROCESSORS_ONLN) : n_writers;
        auto items = generateItems(size);
        long long dt = 0;
        std::cout << "Write test\n" << "writers: " << n_writers<< "  size: " << size << std::endl;
        for (int i = 0; i < numOfTests; ++i) {
            OptSet<int> set;
            dt += writerTest(n_writers, set, items);
            std::string result = "success";
            for (int item : items) {
                if (!set.contains(item)) {
                    result = "fail";
                    break;
                }
            }
            std::cout << "test " << i << ": " << result << std::endl;
        }
        std::cout << "---time: " << dt / numOfTests << " ms" << std::endl;
    }

    static void readTesting(int n_readers, int size, int numOfTests) {
        n_readers = n_readers > sysconf(_SC_NPROCESSORS_ONLN) ? sysconf(_SC_NPROCESSORS_ONLN) : n_readers;
        std::vector<OptSet<int>> sets(numOfTests);
        for (auto& set : sets)
            for (int i = 0; i < size; ++i)
                set.add(i);
        std::vector<int> checkArr(size, 0);
        long long dt = 0;
        std::cout << "Read test\n" <<  "readers: " << n_readers << "  size: " << size << std::endl;
        int i = 0;
        for (auto& set : sets){
            dt += readerTest(n_readers, set, checkArr);
            std::string result = "success";
            if (!set.empty()) {
                result = "fail: not empty";
            }
            else {
                for (int check : checkArr) {
                if (check == 0) {
                        result = "fail";
                        break;
                    }
                }
            }
            std::cout << "test "<< i  << ": "<< result << std::endl;
            i++;
        }
        std::cout << "---time: " << dt / numOfTests << " ms" << std::endl;
    }

    static void writeAndReadTesting(int readersCnt,int writersCnt, int size, int numOfTests) {
        auto items = generateItems(size);
        if( readersCnt + writersCnt > sysconf(_SC_NPROCESSORS_ONLN)){
            int maxthreadsCnt = sysconf(_SC_NPROCESSORS_ONLN);
            readersCnt = (maxthreadsCnt - maxthreadsCnt%2)/2;
            writersCnt = readersCnt;
        }
        std::cout << "Write and read test\n" << "readers: " << readersCnt <<"  writers: " << writersCnt << "  size: " << size << std::endl;
        long long dt = 0;
        for (int j = 0; j < numOfTests; ++j) {
            OptSet<int> set;
            for (int i = 0; i < size; ++i)
                set.add(i);
            std::vector<int> checkArr(size, 0);
            dt += generalTest(readersCnt, writersCnt, set, items, checkArr);
            std::string result = "success";
            for (int i = 0; i < size; i++) {
                if (set.contains(items[i]))
                    continue;
                else if (checkArr[i])
                    continue;
                else {
                    result = "fail";
                    break;
                }
            }
            std::cout << "test "<< j << ": "<< result << std::endl;
        }
        std::cout <<  "---time: " << dt / numOfTests << " ms" << std::endl;
    }
};

pthread_cond_t Tester::cv = PTHREAD_COND_INITIALIZER;
bool Tester::start = false;
