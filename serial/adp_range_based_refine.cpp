//
// Created by 田佳业 on 2022/4/30.
//
#include <iostream>
#include "../readdata.h"

using namespace std;
POSTING_LIST *posting_list_container = (struct POSTING_LIST *) malloc(POSTING_LIST_NUM * sizeof(struct POSTING_LIST));
vector<vector<int> > query_list_container;
MyTimer time_get_intersection;

int QueryNum = 500;

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
        else if (list[mid] <= element)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return low;
}

int serial_search_low(unsigned *list, unsigned int element, int start, int end) {
    while (start <= end) {
        if (list[start] >= element)
            return start;
        else
            start++;
    }
    return -1;
}

int serial_search_high(unsigned *list, unsigned int element, int start, int end) {
    while (start <= end) {
        if (list[end] <= element)
            return end;
        else
            end--;
    }
    return -1;
}


bool not_empty(vector<int> &start, vector<int> &end) {
    for (int i = 0; i < start.size(); i++) {
        if (start[i] > end[i]) {
            return false;
        }
    }
    return true;
}

void adp_range_based(POSTING_LIST *queried_posting_list, int query_word_num, vector<unsigned int> &result_list) {

    //get the key element from the list which just failed to find in the binary search each time
    //start with sorting the posting list to find the shortest one
    int *sorted_index = new int[query_word_num];
    get_sorted_index(queried_posting_list, query_word_num, sorted_index);
    vector<int> start_index_container(query_word_num, 0);
    vector<int> end_index_container(query_word_num, 0);
    for (int i = 0; i < query_word_num; i++) {
        end_index_container[i] = queried_posting_list[sorted_index[i]].len - 1;
    }
    //get "biggest head" list
    unsigned int biggest_head = 0;
    unsigned int biggest_head_index = 0;
    for (int i = 0; i < query_word_num; i++) {
        if (queried_posting_list[sorted_index[i]].arr[start_index_container[i]] > biggest_head) {
            biggest_head = queried_posting_list[sorted_index[i]].arr[start_index_container[i]];
            biggest_head_index = i;
        }
    }
    //get "smallest tail" list
    unsigned int smallest_tail = UINT32_MAX;
    unsigned int smallest_tail_index = 0;
    for (int i = 0; i < query_word_num; i++) {
        if (queried_posting_list[sorted_index[i]].arr[end_index_container[i]] < smallest_tail) {
            smallest_tail = queried_posting_list[sorted_index[i]].arr[end_index_container[i]];
            smallest_tail_index = i;
        }
    }
    //search the biggest head in other lists
    bool flag = false;
    for (int i = 0; i < query_word_num; i++) {
        if (i == biggest_head_index) continue;
        start_index_container[i] = binary_search_higher_position(queried_posting_list[sorted_index[i]].arr, biggest_head, 0, end_index_container[i]);
        if (queried_posting_list[sorted_index[i]].arr[start_index_container[i]] != biggest_head) {
            flag = true;
        } else {
            start_index_container[i]++;
        }
    }
    start_index_container[biggest_head_index]++;
    if (!flag) {
        result_list.push_back(biggest_head);
    }
    //search the smallest tail in other lists
    flag = false;
    for (int i = 0; i < query_word_num; i++) {
        if (i == smallest_tail_index) continue;
        end_index_container[i] = binary_search_lower_position(queried_posting_list[sorted_index[i]].arr, smallest_tail, start_index_container[i], end_index_container[i]);
        if (queried_posting_list[sorted_index[i]].arr[end_index_container[i]] != smallest_tail) {
            flag = true;
        } else {
            end_index_container[i]--;
        }
    }
    end_index_container[smallest_tail_index]--;
    if (!flag) {
        result_list.push_back(smallest_tail);
    }
    if(!not_empty(start_index_container, end_index_container))
        goto end;
    while (true) {
        //get "biggest head" list
        unsigned int biggest_head = 0;
        unsigned int biggest_head_index = 0;
        for (int i = 0; i < query_word_num; i++) {
            if (queried_posting_list[sorted_index[i]].arr[start_index_container[i]] > biggest_head) {
                biggest_head = queried_posting_list[sorted_index[i]].arr[start_index_container[i]];
                biggest_head_index = i;
            }
        }
        //get "smallest tail" list
        unsigned int smallest_tail = UINT32_MAX;
        unsigned int smallest_tail_index = 0;
        for (int i = 0; i < query_word_num; i++) {
            if (queried_posting_list[sorted_index[i]].arr[end_index_container[i]] < smallest_tail) {
                smallest_tail = queried_posting_list[sorted_index[i]].arr[end_index_container[i]];
                smallest_tail_index = i;
            }
        }
        //search the biggest head in other lists
       bool flag = false;
        for (int i = 0; i < query_word_num; i++) {
            if (i == biggest_head_index) continue;
            start_index_container[i] = serial_search_low(queried_posting_list[sorted_index[i]].arr, biggest_head, start_index_container[i],
                                                         end_index_container[i]);
            if(start_index_container[i] == -1)
                goto end;
            if (queried_posting_list[sorted_index[i]].arr[start_index_container[i]] != biggest_head) {
                flag = true;
            } else {
                start_index_container[i]++;
            }
        }
        start_index_container[biggest_head_index]++;
        if (!flag) {
            result_list.push_back(biggest_head);
        }
        //search the smallest tail in other lists
        flag = false;
        for (int i = 0; i < query_word_num; i++) {
            if (i == smallest_tail_index) continue;
            end_index_container[i] = serial_search_high(queried_posting_list[sorted_index[i]].arr, smallest_tail, start_index_container[i],
                                                        end_index_container[i]);
            if(end_index_container[i] == -1)
                goto end;
            if (queried_posting_list[sorted_index[i]].arr[end_index_container[i]] != smallest_tail) {
                flag = true;
            } else {
                end_index_container[i]--;
            }
        }
        end_index_container[smallest_tail_index]--;
        if (!flag) {
            result_list.push_back(smallest_tail);
        }

    }
    end:
    delete[] sorted_index;
}

void query_starter(vector<vector<unsigned int>> &Seq_result) {

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
        vector<unsigned int> Seq_result_list;
        adp_range_based(queried_posting_list, query_word_num, Seq_result_list);
        Seq_result.push_back(Seq_result_list);
        Seq_result_list.clear();
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
        vector<vector<unsigned int>> Seq_result;
        query_starter(Seq_result);
        //test the correctness of the result
        for (int j = 0; j < 5; ++j) {
            printf("result %d: ", j);
            printf("%zu\n", Seq_result[j].size());
            for (unsigned int k: Seq_result[j]) {
                printf("%d ", k);
            }
            printf("\n");
        }

        time_get_intersection.get_duration("adp_range_based");
        free(posting_list_container);
        return 0;
    }
}

