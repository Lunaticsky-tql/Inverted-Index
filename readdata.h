//
// Created by 田佳业 on 2022/4/27.
//

#include <cstdio>
#include <cstdlib>
#include <vector>
#include "./timer.h"
using namespace std;

#ifndef PTHREAD_READDATA_H
#define PTHREAD_READDATA_H

#endif

struct POSTING_LIST {
    unsigned int len;
    unsigned int *arr;
};
const int POSTING_LIST_NUM = 1756;
int posting_list_counter, query_list_count;
FILE *fi;
FILE *fq;
MyTimer time_read_posting_list;
MyTimer time_read_query_list;


vector<int> to_int_list(const char *line) {
    vector<int> int_list;
    int i = 0;
    int num = 0;
    while (line[i] == ' ' || (line[i] >= 48 && line[i] <= 57)) {
        num = 0;
        while (line[i] != ' ') {
            num *= 10;
            int tmp = line[i] - 48;
            num += tmp;
            i++;
        }
        i++;
        int_list.push_back(num);
    }
    return int_list;
}
void device_information() {

#if defined  __aarch64__
    printf("this is aarch64\n");
#elif defined  __x86_64__
    printf("this is x86_64\n");
#endif
}
int read_posting_list(struct POSTING_LIST* posting_list_container) {


    device_information();
    time_read_posting_list.start();
    unsigned int array_len;
    unsigned int *temp_arr;
#if defined  __aarch64__
    fi = fopen("/Users/tianjiaye/CLionProjects/ParallelProgramming/ExpIndex", "rb");
#elif defined  __x86_64__
    fi = fopen(R"(C:\Users\LENOVO\CLionProjects\ParallelProgramming\ExpIndex)", "rb");
#endif

    if (nullptr == fi) {
        printf("Can not open file ExpIndex!\n");
        return 1;
    }
    if (nullptr == posting_list_container) {
        printf("Can not malloc enough space\n");
        return 2;
    }
    while (true) {
        fread(&array_len, sizeof(unsigned int), 1, fi);
        if (feof(fi)) break;
        temp_arr = (unsigned int *) malloc(array_len * sizeof(unsigned int));
        for (int i = 0; i < array_len; i++) {
            fread(&temp_arr[i], sizeof(unsigned int), 1, fi);
            if (feof(fi)) break;
        }
        if (feof(fi)) break;
        POSTING_LIST tmp{};
        tmp.len = array_len;
        tmp.arr = temp_arr;
        posting_list_container[posting_list_counter].len = array_len;
        posting_list_container[posting_list_counter].arr = temp_arr;
        posting_list_counter++;

    }
    fclose(fi);
    time_read_posting_list.finish();
    time_read_posting_list.get_duration("read_posting_list");
    return 0;

}

int read_query_list(vector<vector<int> > &query_list_container) {
    time_read_query_list.start();
#if defined  __aarch64__
    fq = fopen("/Users/tianjiaye/CLionProjects/ParallelProgramming/ExpQuery", "r");
#elif defined  __x86_64__
    fq = fopen(R"(C:\Users\LENOVO\CLionProjects\ParallelProgramming\ExpQuery)", "r");
#endif


    if (nullptr == fq) {
        printf("Can not open file ExpQuery!\n");
        return 1;
    }
    vector<int> query_list;
    char *line = new char[100];
    while ((fgets(line, 100, fq)) != nullptr) {
        query_list = to_int_list(line);
        query_list_container.push_back(query_list);
        query_list_count++;
    }
    fclose(fq);
    time_read_query_list.finish();
    time_read_query_list.get_duration("read_query_list");
    return 0;
}