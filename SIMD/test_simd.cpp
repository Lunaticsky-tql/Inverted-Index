#include <iostream>
#include <immintrin.h>
#include <smmintrin.h>
#include<tmmintrin.h>
#include <xmmintrin.h>
#include <vector>
using namespace std;
int main() {
    //test the correctness of the simd zip_zap
unsigned int *list=new unsigned int[4]{2,5,11,14};
vector<unsigned int> vector_list={1,2,14,17};
    __m128i v_a = _mm_loadu_si128((__m128i*)list);
    __m128i v_b=_mm_loadu_si128((__m128i*)vector_list.data());
    int a_max=_mm_extract_epi32(v_a,3);
    int b_max=_mm_extract_epi32(v_b,3);
    printf("%d\n",a_max);
    printf("%d\n",b_max);
    __m128i cmp1=_mm_cmpeq_epi32(v_a, v_b);
    v_b=_mm_shuffle_epi32(v_b, _MM_SHUFFLE(0,3,2,1));
    __m128i cmp2=_mm_cmpeq_epi32(v_a, v_b);
    v_b=_mm_shuffle_epi32(v_b, _MM_SHUFFLE(0,3,2,1));
    __m128i cmp3=_mm_cmpeq_epi32(v_a, v_b);
    v_b=_mm_shuffle_epi32(v_b, _MM_SHUFFLE(0,3,2,1));
    __m128i cmp4=_mm_cmpeq_epi32(v_a, v_b);
    __m128i cmp_mask=_mm_or_si128(_mm_or_si128(cmp1,cmp2),_mm_or_si128(cmp3,cmp4));
    int mask = _mm_movemask_ps((__m128)cmp_mask);
    printf("%d\n",mask);
    //print the according element in v_a
    mask&1<<0?printf("%d\n",_mm_extract_epi32(v_a,0)):printf("");
    mask&1<<1?printf("%d\n",_mm_extract_epi32(v_a,1)):printf("");
    mask&1<<2?printf("%d\n",_mm_extract_epi32(v_a,2)):printf("");
    mask&1<<3?printf("%d\n",_mm_extract_epi32(v_a,3)):printf("");

    return 0;
}
