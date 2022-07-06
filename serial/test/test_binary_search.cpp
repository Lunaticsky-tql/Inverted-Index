//
// Created by LENOVO on 2022/7/4.
//

#include <iostream>
using namespace std;

int binary_search_return_lower_bound(const int *p, int element) {

    int low = 0, high =9, mid;
    while (low <= high) {
        mid = (low + high) / 2;
        if (p[mid] == element)
            return mid;
        else if (p[mid] <= element)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return high;
}

int binary_search_return_higher_bound(const int *p, int element) {

    int low = 0, high = 9, mid;
    while (low <= high) {
        mid = (low + high) / 2;
        if (p[mid] == element)
            return mid;
        else if (p[mid] <=element)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return low;
}

int main() {
    //test two different ways of binary search
    int a[10] = {2,4,6,8,10,12,14,16,18,20};
    int searching_element = 21;
    int index1 = 0;
    int index2 = 0;
    index1=binary_search_return_lower_bound(a, searching_element);
    index2=binary_search_return_higher_bound(a, searching_element);
    printf("%d %d\n", index1, index2);
    printf("%d %d\n", a[index1], a[index2]);
    cout<<a[10]<<endl;
    cout<<a[11]<<endl;
}
