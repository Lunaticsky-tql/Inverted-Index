//
// Created by 田佳业 on 2022/4/30.
//
#include <iostream>
#include <immintrin.h>
#include "../readdata.h"
#include <mpi.h>
using namespace std;
POSTING_LIST *posting_list_container = (struct POSTING_LIST *) malloc(POSTING_LIST_NUM * sizeof(struct POSTING_LIST));
vector<vector<int> > query_list_container;
MyTimer time_get_intersection;

int QueryNum = 500;
int MY_RANK, SIZE;
void get_sorted_index(POSTING_LIST *queried_posting_list, int query_word_num, int *sorted_index) {

    for (int i = 0; i < query_word_num; i++) {
        sorted_index[i] = i;
    }
    for (int i = 0; i < query_word_num - 1; i++) {
        for (int j = i + 1; j < query_word_num; j++) {
            if (queried_posting_list[sorted_index[i]].len > queried_posting_list[sorted_index[j]].len) {
                int temp = sorted_index[i];
                sorted_index[i] = sorted_index[j];
                sorted_index[j] = temp;
            }
        }
    }
}

int binary_search_with_position(POSTING_LIST *list, unsigned int element, int index) {
    int low = index, high = list->len - 1, mid;
    while (low <= high) {
        mid = (low + high) / 2;
        if (list->arr[mid] == element)
            return mid;
        else if (list->arr[mid] < element)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return low;
}

int serial_search_with_location(POSTING_LIST *list, unsigned int element, int index) {
    while(index < list->len) {
        if (list->arr[index] >= element)
            return index;
        else
            index++;
    }
    return index;
}

int serial_search_with_location_using_SIMD(POSTING_LIST *list, unsigned int element, int index) {
    const __m128i keys = _mm_set1_epi32(element);

    const auto len_list = list->len;
    const auto remainder = len_list % 4;

    for (size_t i=index; i < len_list-remainder; i += 8) {

        const __m128i vec1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&list->arr[i]));
        const __m128i vec2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&list->arr[i + 4]));
        const __m128i cmp1 = _mm_cmpgt_epi32(vec1, keys);
        const __m128i cmp2 = _mm_cmpgt_epi32(vec2, keys);
        const __m128i tmp  = _mm_packs_epi32(cmp1, cmp2);
        const uint32_t mask = _mm_movemask_epi8(tmp);

        if (mask != 0) {
            int pos = i + __builtin_ctz(mask)/2;
            if(pos==index)
                return index;
            else if(list->arr[pos-1]==element)
                return pos-1;
            else
                return pos;
        }
    }

    for (size_t i=len_list-remainder; i < len_list; i++) {
        if (list->arr[i] >= element)
            return i;
    }
    return list->len;
}
void max_successor(POSTING_LIST *queried_posting_list, int query_word_num, vector<unsigned int> &result_list) {
    //get the key element from the list which just failed to find in the binary search each time
    //we should additionally compare the first element of the list with the key element get from compare miss
    //choose the larger one as the key element in the next turn

    //start with sorting the posting list to find the shortest list
    int *sorted_index = new int[query_word_num];
    get_sorted_index(queried_posting_list, query_word_num, sorted_index);
    bool flag;
    unsigned int key_element;
    vector<int> finding_pointer(query_word_num, 0);
    key_element = queried_posting_list[sorted_index[0]].arr[finding_pointer[sorted_index[0]]];
    int gaping_mth_short = 0;
    while (true) {
        flag = true;
        for (int m = 0; m < query_word_num; ++m) {
            if (m == gaping_mth_short)
                continue;
            else {
                int mth_short = sorted_index[m];
                POSTING_LIST searching_list = queried_posting_list[mth_short];
//                int location = binary_search_with_position(&queried_posting_list[mth_short], key_element,
//                                                           finding_pointer[sorted_index[m]]);
//                int location = serial_search_with_location(&queried_posting_list[mth_short], key_element,
//                                                           finding_pointer[sorted_index[m]]);
                int location = serial_search_with_location_using_SIMD(&queried_posting_list[mth_short], key_element,
                                                                      finding_pointer[sorted_index[m]]);
                if (searching_list.arr[location] != key_element) {
                    if (searching_list.len == location) {
                        //all the elements in the list are smaller than the key element, algorithm end
                        goto end_Max;
                    }
                    flag = false;

                    if (queried_posting_list[sorted_index[0]].arr[finding_pointer[sorted_index[0]] + 1] >
                        searching_list.arr[location])
                    {
                        key_element = queried_posting_list[sorted_index[0]].arr[++finding_pointer[sorted_index[0]]];
                        gaping_mth_short = 0;
                    } else {
                        key_element = searching_list.arr[location];
                        //this is optional, and it just allows us to find one less element in the first list.
                        finding_pointer[sorted_index[0]]++;
                        gaping_mth_short = m;
                    }
                    finding_pointer[mth_short] = location;
                    break;
                }
                finding_pointer[mth_short] = location;
            }

        }
        if (flag) {
            result_list.push_back(key_element);
//            finding_pointer[sorted_index[0]]++;
            key_element = queried_posting_list[sorted_index[0]].arr[++finding_pointer[sorted_index[0]]];
            gaping_mth_short = 0;
        }
        if (finding_pointer[sorted_index[0]] ==
            queried_posting_list[sorted_index[0]].len) {
            //all the elements in the list are smaller than the key element, algorithm end
            goto end_Max;
        }
    }
    end_Max:
    delete[] sorted_index;
}

void query_starter(vector<vector<unsigned int>> &Max_result) {
    MPI_Comm_size(MPI_COMM_WORLD, &SIZE);
    MPI_Comm_rank(MPI_COMM_WORLD, &MY_RANK);
    int divider=QueryNum/SIZE;

    time_get_intersection.start();
    for (int i = MY_RANK*divider; i <(MY_RANK+1)*divider; i++) {
        int query_word_num = query_list_container[i].size();
        //get the posting list of ith query
        auto *queried_posting_list = new POSTING_LIST[query_word_num];
        for (int j = 0; j < query_word_num; j++) {
            int query_list_item = query_list_container[i][j];
            queried_posting_list[j] = posting_list_container[query_list_item];
        }
        //get the result of ith query
        vector<unsigned int> Max_result_list;
        max_successor(queried_posting_list, query_word_num, Max_result_list);
        Max_result.push_back(Max_result_list);
        Max_result_list.clear();
        delete[] queried_posting_list;
    }
    time_get_intersection.finish();
}

int main() {

    if (read_posting_list(posting_list_container) || read_query_list(query_list_container)) {
        printf("read_posting_list failed\n");
        free(posting_list_container);
        return -1;
    } else {
        printf("query_num: %d\n", QueryNum);
        MPI_Init(NULL, NULL);
        vector<vector<unsigned int>> Max_result;
        query_starter(Max_result);
        if(MY_RANK==0)
        {
            for (int j = 0; j < 5; ++j) {
                printf("query %d:", j);
                printf("%zu\n", Max_result[j].size());
                for (unsigned int k: Max_result[j]) {
                    printf("%d ", k);
                }
                printf("\n");
            }
        }
        double my_time=time_get_intersection.get_duration("max_mpi");
        //get the max time
        double max_time=0;
        MPI_Reduce(&my_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        if(MY_RANK==0)
        {
            printf("max time used in devices: %f\n", max_time);
        }
        free(posting_list_container);
        MPI_Finalize();
        return 0;
    }
}


