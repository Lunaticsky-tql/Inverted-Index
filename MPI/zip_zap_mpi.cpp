#include <iostream>
#include <mpi.h>
#include "../readdata.h"

using namespace std;
POSTING_LIST *posting_list_container = (struct POSTING_LIST *) malloc(POSTING_LIST_NUM * sizeof(struct POSTING_LIST));
vector<vector<int>> query_list_container;
MyTimer time_get_intersection;
static int search_time = 0;
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
    vector<unsigned int> temp_result_list;
    int l0_len = queried_posting_list[sorted_index[0]].len;
    int divider = l0_len / SIZE;
    int start_pos, end_pos;
    if (MY_RANK == SIZE - 1) {
        start_pos = MY_RANK * divider;
        end_pos = l0_len;
    } else {
        start_pos = MY_RANK * divider;
        end_pos = (MY_RANK + 1) * divider;
    }
    for(int i = start_pos; i < end_pos; i++) {
        temp_result_list.push_back(queried_posting_list[sorted_index[0]].arr[i]);
    }

    for (int k = 1; k < query_word_num; k++) {
        vector<unsigned int> svs_temp_result_list;
        unsigned int p0 = 0;
        unsigned int pi = 0;
        while (p0 <temp_result_list.size() && pi < queried_posting_list[sorted_index[k]].len) {
            if (temp_result_list[p0] == queried_posting_list[sorted_index[k]].arr[pi]) {
                svs_temp_result_list.push_back(temp_result_list[p0]);
                p0++;
                pi++;
            } else if (temp_result_list[p0] < queried_posting_list[sorted_index[k]].arr[pi]) {
                p0++;
            } else {
                pi++;
            }
        }
        temp_result_list = svs_temp_result_list;
    }
    if(MY_RANK == 0) {
        result_list.assign(temp_result_list.begin(), temp_result_list.end());
        for(int i = 1; i < SIZE; i++) {
            int size=0;
            MPI_Recv(&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            temp_result_list.clear();
            temp_result_list.resize(size);
            MPI_Recv(&temp_result_list[0], size, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            result_list.insert(result_list.end(), temp_result_list.begin(), temp_result_list.end());
        }
    }
    else
    {
        int temp_list_size = temp_result_list.size();
        MPI_Send(&temp_list_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&temp_result_list[0], temp_list_size, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD);
    }

}

void svs_zip_zap_starter(vector<vector<unsigned int>> &svs_result) {
    MPI_Comm_rank(MPI_COMM_WORLD, &MY_RANK);
    MPI_Comm_size(MPI_COMM_WORLD, &SIZE);
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
        MPI_Init(NULL, NULL);
        vector<vector<unsigned int>> svs_result;
        svs_zip_zap_starter(svs_result);
        //test the correctness of the result
        if(MY_RANK==0)
        {
            for (int j = 0; j < 5; ++j) {
                printf("query %d:", j);
                printf("%zu\n", svs_result[j].size());
                for (unsigned int k: svs_result[j]) {
                    printf("%d ", k);
                }
                printf("\n");
            }
        }

        double my_time=time_get_intersection.get_duration("zip_zap");
        //get the max time
        double max_time=0;
        MPI_Reduce(&my_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        if(MY_RANK==0)
        {
            printf("zip_zap max time used in devices: %f\n", max_time);
        }
        free(posting_list_container);
        MPI_Finalize();
        return 0;
    }
}

