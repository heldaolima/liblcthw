#include <lcthw/darray_algos.h>
#include <stdlib.h>
#include <string.h>

void swap(void* v1, void* v2, int size)
{
    char buffer[size];
  
    memcpy(buffer, v1, size);
    memcpy(v1, v2, size);
    memcpy(v2, buffer, size);
}

void _qsort(void* v, int size, int left, int right,
                      int (*comp)(void*, void*))
{
    void *vt, *v3;
    int i, last, mid = (left + right) / 2;
    if (left >= right)
        return;
  
    void* vl = (char*)(v + (left * size));
    void* vr = (char*)(v + (mid * size));
    swap(vl, vr, size);
    last = left;
    for (i = left + 1; i <= right; i++) {
        vt = (char*)(v + (i * size));
        if ((*comp)(vl, vt) > 0) {
            ++last;
            v3 = (char*)(v + (last * size));
            swap(vt, v3, size);
        }
    }
    v3 = (char*)(v + (last * size));
    swap(vl, v3, size);
    _qsort(v, size, left, last - 1, comp);
    _qsort(v, size, last + 1, right, comp);
}

int DArray_qsort(DArray *array, DArray_compare cmp)
{
    _qsort(array->contents, sizeof(void*), 0, DArray_count(array)-1, cmp);
    return 0;
}

void heapify(void* base, size_t size, size_t N, size_t i, DArray_compare cmp)
{
    size_t largest = i;
    size_t left = 2 * i + 1;
    size_t right = 2 * i + 1;

    void* arr_i = (char*) (base + (i * size));
    void* arr_largest = arr_i;
    void* arr_left = (char*) (base + (left * size));
    void* arr_right = (char*) (base + (right * size));

    if (left < N && cmp(arr_left, arr_largest) > 0) {
        arr_largest = arr_left;
        largest = left;
    }

    if (right < N && cmp(arr_right, arr_largest) > 0) {
        arr_largest = arr_right;
        largest = right;
    }

    if (largest != i) {
        swap(arr_i, arr_largest, size);
        heapify(base, size, N, largest, cmp);
    }
}

void _heapsort(void* base, size_t size, int N, DArray_compare cmp)
{
    int i = 0;
    void* arr_0 = NULL;
    void* arr_i = NULL;
    for (i = N / 2 - 1; i >= 0; i--) {
        heapify(base, size, N, i, cmp);
    }

    for (i = N - 1; i >= 0; i--) {
        arr_0 = (char*) (base + (0 * size));
        arr_i = (char*) (base + (i * size));
        swap(arr_0, arr_i, size);

        heapify(base, size, i, 0, cmp);
    }
}

int DArray_heapsort(DArray *array, DArray_compare cmp)
{
    _heapsort(array->contents, sizeof(void*), DArray_count(array), cmp);
    return 0;
    // return heapsort(array->contents, DArray_count(array), sizeof(void *), cmp); // non ecziste in stdlib
}

int DArray_mergesort(DArray *array, DArray_compare cmp)
{
    return 0;
    // return mergesort(array->contents, DArray_count(array), sizeof(void *), cmp); // non ecziste in stdlib
}