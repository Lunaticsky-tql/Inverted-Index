#include <iostream>
#include <immintrin.h>
#include <smmintrin.h>
#include<tmmintrin.h>
#include <xmmintrin.h>

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


void SvS_zip_zap(POSTING_LIST *queried_posting_list, int query_word_num, vector<unsigned int> &result_list) {
    int *sorted_index = new int[query_word_num];
    get_sorted_index(queried_posting_list, query_word_num, sorted_index);
    //Put the shortest inverted list at the front
    for (int i = 0; i < queried_posting_list[sorted_index[0]].len; i++) {
        result_list.push_back(queried_posting_list[sorted_index[0]].arr[i]);
    }
    for (int i = 1; i < query_word_num; i++) {
        vector<unsigned int> temp_result_list;
        unsigned int *temp_array = queried_posting_list[sorted_index[i]].arr;
        unsigned int rounded_len0 = (queried_posting_list[sorted_index[0]].len / 4) * 4;
        unsigned int rounded_leni = (queried_posting_list[sorted_index[i]].len / 4) * 4;
        unsigned int p0 = 0, pi = 0;
        while (p0 < rounded_len0 && pi < rounded_leni) {
//load the result array in unsigned int vector register
            __m128i v_0 = _mm_loadu_si128(reinterpret_cast<__m128i *>(result_list.data() + p0));
            __m128i v_i = _mm_loadu_si128(reinterpret_cast<__m128i *>(temp_array + pi));
            //get the last element of v0 and vi in an unsigned int
            int l0_max=_mm_extract_epi32(v_0,3);
            int li_max=_mm_extract_epi32(v_i,3);
            //move pointers
            p0+=(l0_max<=li_max)?4:0;
            pi+=(li_max<=l0_max)?4:0;
            //compute mask of common elements in v0 and vi with cycle shift
            __m128i v_mask1 = _mm_cmpeq_epi32(v_0, v_i);
            v_i = _mm_shuffle_epi32(v_i, _MM_SHUFFLE(0,3,2,1));
            __m128i v_mask2 = _mm_cmpeq_epi32(v_0, v_i);
            v_i = _mm_shuffle_epi32(v_i, _MM_SHUFFLE(0,3,2,1));
            __m128i v_mask3 = _mm_cmpeq_epi32(v_0, v_i);
            v_i = _mm_shuffle_epi32(v_i, _MM_SHUFFLE(0,3,2,1));
            __m128i v_mask4 = _mm_cmpeq_epi32(v_0, v_i);
            __m128i cmp_mask=_mm_or_si128(_mm_or_si128(v_mask1,v_mask2),_mm_or_si128(v_mask3,v_mask4));
            int mask = _mm_movemask_ps((__m128)cmp_mask);
            //get the position of the common elements in v0 and vi
            if(mask!=0) {
                if(mask&1<<0)
                    temp_result_list.push_back(_mm_extract_epi32(v_0,0));
                if(mask&1<<1)
                    temp_result_list.push_back(_mm_extract_epi32(v_0,1));
                if(mask&1<<2)
                    temp_result_list.push_back(_mm_extract_epi32(v_0,2));
                if(mask&1<<3)
                    temp_result_list.push_back(_mm_extract_epi32(v_0,3));
            }
        }
        //process the remaining elements in v0
        while (p0 < result_list.size() && pi < queried_posting_list[sorted_index[i]].len) {
            if (result_list[p0] == queried_posting_list[sorted_index[i]].arr[pi]) {
                temp_result_list.push_back(result_list[p0]);
                p0++;
                pi++;
            } else if (result_list[p0] < queried_posting_list[sorted_index[i]].arr[pi]) {
                p0++;
            } else {
                pi++;
            }
        }
        result_list=temp_result_list;
    }
}

void svs_zip_zap_starter(vector<vector<unsigned int>> &svs_result) {
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
        SvS_zip_zap(queried_posting_list, query_word_num, svs_result_list);
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
        svs_zip_zap_starter(svs_result);
        //test the correctness of the result
        for (int j = 0; j < 5; ++j) {
            printf("query %d:", j);
            printf("%zu\n", svs_result[j].size());
            for (unsigned int k: svs_result[j]) {
                printf("%d ", k);
            }
            printf("\n");
        }
        time_get_intersection.get_duration("zip_zap_SIMD");
        free(posting_list_container);
        return 0;
    }
}

