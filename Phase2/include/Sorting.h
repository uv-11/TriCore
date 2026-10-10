#ifndef SORTING_H
#define SORTING_H


template <class T>
bool lessByOperator(const T& a, const T& b) {
    return a < b;
}


template <class T>
void mergeParts(T* a, T* temp, int left, int mid, int right,
                bool (*less)(const T&, const T&)) {
    int i = left;
    int j = mid + 1;
    int k = left;

    while (i <= mid && j <= right) {
        if (less(a[j], a[i])) {
            temp[k] = a[j];
            ++j;
        }
        else {
            temp[k] = a[i];
            ++i;
        }

        ++k;
    }

    while (i <= mid) {
        temp[k] = a[i];
        ++i;
        ++k;
    }

    while (j <= right) {
        temp[k] = a[j];
        ++j;
        ++k;
    }

    for (i = left; i <= right; ++i) {
        a[i] = temp[i];
    }
}


template <class T>
void mergeSortRange(T* a, T* temp, int left, int right, bool (*less)(const T&, const T&)) {
    if (left >= right) {
        return;
    }

    int mid = left + (right - left) / 2;

    mergeSortRange(a, temp, left, mid, less);
    mergeSortRange(a, temp, mid + 1, right, less);

    mergeParts(a, temp, left, mid, right, less);
}


template <class T>
void mergeSort(T* a, int n, bool (*less)(const T&, const T&)) {
    if (a == NULL || n <= 1) {
        return;
    }

    T* temp = new T[n];

    mergeSortRange(a, temp, 0, n - 1, less);

    delete[] temp;
}


template <class T>
void mergeSort(T* a, int n)
{
    mergeSort(a, n, lessByOperator<T>);
}

#endif