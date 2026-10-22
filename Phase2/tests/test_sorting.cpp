#include "Sorting.h"
#include "RecommendationEngine.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>

using namespace std;

void check(bool ok, const string& name){
    if (!ok)
    {
        cout << "FAIL: " << name << endl;
        exit(1);
    }
}

void checkIntArray(const int* a, const int* b, int n, const string& name) {
    for (int i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            cout << "FAIL: " << name << endl;
            exit(1);
        }
    }
}

void testSmall(){
    int empty[1] = {7};
    mergeSort(empty, 0);
    check(empty[0] == 7, "SO1 empty");

    int one[1] = {4};
    mergeSort(one, 1);
    check(one[0] == 4, "SO1 one element");
}

void testGeneralSorting() {
    const int sizes[] = {2, 9, 1000};

    for (int s = 0; s < 3; ++s) {
        int n = sizes[s];
        vector<int> a(n);
        vector<int> b(n);

        for (int i = 0; i < n; ++i) {
            a[i] = (i * 37 + 11) % 101;
            b[i] = a[i];
        }

        mergeSort(&a[0], n);
        sort(b.begin(), b.end());

        checkIntArray(&a[0], &b[0], n, "SO1 random");

        for (int i = 0; i < n; ++i) {
            a[i] = i;
            b[i] = i;
        }

        mergeSort(&a[0], n);
        sort(b.begin(), b.end());
        checkIntArray(&a[0], &b[0], n, "SO1 sorted");

        for (int i = 0; i < n; ++i) {
            a[i] = n - i;
            b[i] = n - i;
        }

        mergeSort(&a[0], n);
        sort(b.begin(), b.end());
        checkIntArray(&a[0], &b[0], n, "SO1 reversed");

        for (int i = 0; i < n; ++i) {
            a[i] = i % 4;
            b[i] = i % 4;
        }

        mergeSort(&a[0], n);
        sort(b.begin(), b.end());
        checkIntArray(&a[0], &b[0], n, "SO1 duplicates");
    }
}

struct StableItem
{
    int key;
    int order;

    bool operator<(const StableItem& other) const {
        return key < other.key;
    }
};

void testStability(){
    StableItem a[8] = {
        {2, 0}, {1, 1}, {2, 2}, {1, 3},
        {2, 4}, {1, 5}, {3, 6}, {2, 7}
    };

    mergeSort(a, 8);

    check(a[0].key == 1 && a[0].order == 1, "SO2 first");
    check(a[1].key == 1 && a[1].order == 3, "SO2 second");
    check(a[2].key == 1 && a[2].order == 5, "SO2 third");
    check(a[3].key == 2 && a[3].order == 0, "SO2 fourth");
    check(a[4].key == 2 && a[4].order == 2, "SO2 fifth");
    check(a[5].key == 2 && a[5].order == 4, "SO2 sixth");
    check(a[6].key == 2 && a[6].order == 7, "SO2 seventh");
    check(a[7].key == 3 && a[7].order == 6, "SO2 eighth");
}

bool reverseLess(const int& a, const int& b){
    return a > b;
}

void testComparator(){
    int a[6] = {1, 5, 2, 4, 3, 0};
    mergeSort(a, 6, reverseLess);

    int expected[6] = {5, 4, 3, 2, 1, 0};
    checkIntArray(a, expected, 6, "SO1 comparator");
}

void testCandidateRanking(){
    Candidate s4[3] = {
        Candidate(EntityId("R", 1), 60, Duration(25), 90),
        Candidate(EntityId("R", 4), 60, Duration(12), 20),
        Candidate(EntityId("R", 3), 60, Duration(22), 60)
    };

    mergeSort(s4, 3);

    check(s4[0].recipientId().number() == 4, "SO3 S4 R004");
    check(s4[1].recipientId().number() == 3, "SO3 S4 R003");
    check(s4[2].recipientId().number() == 1, "SO3 S4 R001");

    Candidate s5[3] = {
        Candidate(EntityId("R", 4), 80, Duration(12), 0),
        Candidate(EntityId("R", 1), 100, Duration(25), 50),
        Candidate(EntityId("R", 3), 100, Duration(22), 20)
    };

    mergeSort(s5, 3);

    check(s5[0].recipientId().number() == 3, "SO3 S5 R003");
    check(s5[1].recipientId().number() == 1, "SO3 S5 R001");
    check(s5[2].recipientId().number() == 4, "SO3 S5 R004");

    Candidate s6[2] = {
        Candidate(EntityId("R", 1), 100, Duration(25), 50),
        Candidate(EntityId("R", 2), 100, Duration(15), 200)
    };

    mergeSort(s6, 2);

    check(s6[0].recipientId().number() == 2, "SO3 S6 R002");
    check(s6[1].recipientId().number() == 1, "SO3 S6 R001");
}

void testComparatorProperties() {
    Candidate a(EntityId("R", 7), 10, Duration(5), 2);
    Candidate b(EntityId("R", 7), 10, Duration(5), 2);
    Candidate c(EntityId("R", 8), 10, Duration(5), 2);

    check(!(a < a), "R6 irreflexive");
    check(!(a < b), "R6 equal candidates");
    check(a < c, "R6 recipient tie break");
    check(!(c < a), "R6 recipient reverse");
}

int main() {
    testSmall();
    testGeneralSorting();
    testStability();
    testComparator();
    testCandidateRanking();
    testComparatorProperties();

    cout << "All sorting tests passed." << endl;
    return 0;
}