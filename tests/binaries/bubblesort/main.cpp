#include <stdio.h>
#include <vector>
#include <stdlib.h>

void bubblesort(std::vector<int>& vec) {
    for (int i = vec.size() - 1; i >= 0; --i) {
        for (int j = i; j < vec.size() - 1; ++j) {
            if (vec[j] > vec[j + 1]) {
                std::swap(vec[j], vec[j + 1]);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    printf("args: ");
    for (int i = 1; i < argc; ++i) {
        printf("%s ", argv[i]);
    }
    printf("\n");
    
    std::vector<int> v;
    for (int i = 1; i < argc; ++i) {
        v.push_back(atoi(argv[i]));
    }
    bubblesort(v);
    printf("sorted: ");
    for (unsigned i = 0; i < v.size(); ++i) {
        printf("%d ", v[i]);
    }
    printf("\n");
}
