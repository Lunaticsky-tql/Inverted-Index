#include <iostream>
#include<algorithm>
#include "../readdata.h"

using namespace std;
POSTING_LIST *posting_list_container = (struct POSTING_LIST *) malloc(POSTING_LIST_NUM * sizeof(struct POSTING_LIST));
vector<vector<int>> query_list_container;
MyTimer time_get_intersection;
static int search_time = 0;
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

int binary_search(POSTING_LIST *list, unsigned int element, int index) {
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
    return -1;

}

int binary_search_with_position(POSTING_LIST *list, unsigned int element, int index) {
    search_time++;
    //If found, return the position of the element; otherwise, return the position of the first element that is not less than it
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


void STL_intersection(POSTING_LIST *queried_posting_list, int query_word_num, vector<unsigned int> &result_list) {
    //use STL template to find the intersection of the posting list
//source code below:
//    template <class _InIt1, class _InIt2, class _OutIt, class _Pr>
//    _CONSTEXPR20 _OutIt set_intersection(
//            _InIt1 _First1, _InIt1 _Last1, _InIt2 _First2, _InIt2 _Last2, _OutIt _Dest, _Pr _Pred) {
//        // AND sets [_First1, _Last1) and [_First2, _Last2)
//        _Adl_verify_range(_First1, _Last1);
//        _Adl_verify_range(_First2, _Last2);
//        auto _UFirst1      = _Get_unwrapped(_First1);
//        const auto _ULast1 = _Get_unwrapped(_Last1);
//        auto _UFirst2      = _Get_unwrapped(_First2);
//        const auto _ULast2 = _Get_unwrapped(_Last2);
//        _DEBUG_ORDER_SET_UNWRAPPED(_InIt2, _UFirst1, _ULast1, _Pred);
//        _DEBUG_ORDER_SET_UNWRAPPED(_InIt1, _UFirst2, _ULast2, _Pred);
//        auto _UDest = _Get_unwrapped_unverified(_Dest);
//        while (_UFirst1 != _ULast1 && _UFirst2 != _ULast2) {
//            if (_DEBUG_LT_PRED(_Pred, *_UFirst1, *_UFirst2)) {
//                ++_UFirst1;
//            } else if (_Pred(*_UFirst2, *_UFirst1)) {
//                ++_UFirst2;
//            } else {
//                *_UDest = *_UFirst1;
//                ++_UDest;
//                ++_UFirst1;
//                ++_UFirst2;
//            }
//        }
//
//        _Seek_wrapped(_Dest, _UDest);
//        return _Dest;
//    }
    int *sorted_index = new int[query_word_num];
    get_sorted_index(queried_posting_list, query_word_num, sorted_index);
//    result_list.resize(queried_posting_list[sorted_index[0]].len > queried_posting_list[sorted_index[1]].len
//                       ? queried_posting_list[sorted_index[0]].len : queried_posting_list[sorted_index[1]].len);
//    set_intersection(queried_posting_list[sorted_index[0]].arr,
//                     queried_posting_list[sorted_index[0]].arr + queried_posting_list[sorted_index[0]].len,
//                     queried_posting_list[sorted_index[1]].arr,
//                     queried_posting_list[sorted_index[1]].arr + queried_posting_list[sorted_index[1]].len,
//                     result_list.begin());
//    for (int i = 2; i < query_word_num; i++) {
//        set_intersection(result_list.begin(), result_list.end(),
//                         queried_posting_list[sorted_index[i]].arr,
//                         queried_posting_list[sorted_index[i]].arr + queried_posting_list[sorted_index[i]].len,
//                         result_list.begin());
//    }
//    result_list.resize(distance(result_list.begin(), unique(result_list.begin(), result_list.end())));
for(int i=0;i<queried_posting_list[sorted_index[0]].len;i++)
{
    result_list.push_back(queried_posting_list[sorted_index[0]].arr[i]);
}

for(int i = 1; i < query_word_num; i++) {
    vector<unsigned int> temp_list(queried_posting_list[sorted_index[1]].len, 0);
    auto intersection_end = set_intersection(queried_posting_list[sorted_index[i]].arr,
                                              queried_posting_list[sorted_index[i]].arr + queried_posting_list[sorted_index[i]].len,
                                              result_list.begin(),
                                              result_list.end(),temp_list.begin());
    temp_list.resize(intersection_end - temp_list.begin());
    result_list.swap(temp_list);
}
    delete[] sorted_index;
}

void stl_starter(vector<vector<unsigned int>> &svs_result) {
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
        STL_intersection(queried_posting_list, query_word_num, svs_result_list);
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
        stl_starter(svs_result);
        //test the correctness of the result
        for (int j = 0; j < 5; ++j) {
            printf("query %d:", j);
            printf("%zu\n", svs_result[j].size());
            for (unsigned int k: svs_result[j]) {
                printf("%d ", k);
            }
            printf("\n");
        }
        time_get_intersection.get_duration("stl_intersection");
        free(posting_list_container);
        return 0;
    }
}



