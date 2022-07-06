#include <iostream>
#include <immintrin.h>
#include <smmintrin.h>
#include<tmmintrin.h>
#include <xmmintrin.h>
#include <vector>
#include <algorithm>

#include "../readdata.h"

using namespace std;
POSTING_LIST *posting_list_container = (struct POSTING_LIST *) malloc(POSTING_LIST_NUM * sizeof(struct POSTING_LIST));
vector<vector<int>> query_list_container;
MyTimer time_get_intersection;
int QueryNum = 500;

void get_sorted_index(POSTING_LIST *queried_posting_list, int query_word_num, int *sorted_index) {

    //rewrite to compare the range of the array
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


int binary_search_lower_position(unsigned *list, unsigned int element, int start, int end) {

    //If found, return the position of the element; otherwise, return the position of the first element that is not bigger than it
    int low = start, high = end, mid;
    while (low <= high) {
        mid = (low + high) / 2;
        if (list[mid] == element)
            return mid;
        else if (list[mid] <= element)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return high;
}

int binary_search_higher_position(unsigned *list, unsigned int element, int start, int end) {

    //If found, return the position of the element; otherwise, return the position of the first element that is not less than it
    int low = start, high = end, mid;
    while (low <= high) {
        mid = (low + high) / 2;
        if (list[mid] == element)
            return mid;
        else if (list[mid] <=element)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return low;
}


void SvS_range_based(POSTING_LIST *queried_posting_list, int query_word_num, vector<unsigned int> &result_list) {
    int *sorted_index = new int[query_word_num];
    get_sorted_index(queried_posting_list, query_word_num, sorted_index);
    //Put the shortest inverted list at the front
    for (int i = 0; i < queried_posting_list[sorted_index[0]].len; i++) {
        result_list.push_back(queried_posting_list[sorted_index[0]].arr[i]);
    }
    for (int i = 1; i < query_word_num; i++) {
        vector<unsigned int> temp_result_list;
        int start_0 = 0;
        int end_0 = result_list.size() - 1;
        int start_i = 0;
        int end_i = queried_posting_list[sorted_index[i]].len - 1;
        while (start_0 <= end_0 && start_i <= end_i) {
            //move start pointer
            //"bigger head" list search the start element in the "smaller head" list
            if (result_list[start_0] >= queried_posting_list[sorted_index[i]].arr[start_i])
                start_i = binary_search_higher_position(queried_posting_list[sorted_index[i]].arr, result_list[start_0],
                                                        start_i, end_i);
            else
                start_0 = binary_search_higher_position(&result_list[0],
                                                        queried_posting_list[sorted_index[i]].arr[start_i], start_0,
                                                        end_0);
//            if(start_i>end_i||start_0>end_0)
//                break;
//Without above annotated code,the start_i or start_0 is possible to be out of range.
//But as we only read the values out of range rather than write them,it is safe.
//And we can get 40ms speedup.
            if (result_list[start_0] == queried_posting_list[sorted_index[i]].arr[start_i]) {
                temp_result_list.push_back(result_list[start_0]);
                start_0++;
                start_i++;
            } else {
                result_list[start_0] > queried_posting_list[sorted_index[i]].arr[start_i] ? start_i++ : start_0++;
            }

            //lower tail search the end element in the "bigger tail" list
            if (result_list[end_0] <= queried_posting_list[sorted_index[i]].arr[end_i])
                end_i = binary_search_lower_position(queried_posting_list[sorted_index[i]].arr, result_list[end_0],
                                                     start_i, end_i);
            else
                end_0 = binary_search_lower_position(&result_list[0], queried_posting_list[sorted_index[i]].arr[end_i],
                                                     start_0, end_0);
//            if(start_i>end_i||start_0>end_0)
//                break;
//Without above annotated code,the start_i or start_0 is possible to be out of range.
//But as we only read the values out of range rather than write them,it is safe.
            if (result_list[end_0] == queried_posting_list[sorted_index[i]].arr[end_i]) {
                temp_result_list.push_back(result_list[end_0]);
                end_0--;
                end_i--;
            } else {
                result_list[end_0] < queried_posting_list[sorted_index[i]].arr[end_i] ? end_i-- : end_0--;
            }
        }
        result_list = temp_result_list;
        sort(result_list.begin(), result_list.end());
    }
}

void svs_range_based_starter(vector<vector<unsigned int>> &svs_result) {
    time_get_intersection.start();

    for (int i = 0; i < QueryNum; i++) {
        int query_word_num = query_list_container[i].size();
        //get the posting list of ith query
        auto *queried_posting_list = new POSTING_LIST[query_word_num];
        for (int j = 0; j < query_word_num; j++) {
            int query_list_item = query_list_container[i][j];
            queried_posting_list[j] = posting_list_container[query_list_item];
        }
        //get the result of ith query
        vector<unsigned int> svs_result_list;
        SvS_range_based(queried_posting_list, query_word_num, svs_result_list);
        svs_result.push_back(svs_result_list);
        svs_result_list.clear();
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
        vector<vector<unsigned int>> svs_result;
        svs_range_based_starter(svs_result);
        //test the correctness of the result
        for (int j = 0; j < 5; ++j) {
            printf("query %d:", j);
            printf("%zu\n", svs_result[j].size());
            for (unsigned int k: svs_result[j]) {
                printf("%d ", k);
            }
            printf("\n");
        }
        time_get_intersection.get_duration("svs_range_based");
        free(posting_list_container);
        return 0;
    }
}


