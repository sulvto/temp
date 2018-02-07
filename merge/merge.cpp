#include <iostream>
#include <vector>
#include <queue>

using namespace std;

template <typename T>
void merge(const vector<vector<T>>& v, vector<T> & S) {
    size_t length = 0;
    for (size_t i = 0; i<v.size(); i++) {
        length += v[i].size();
    }
    S.clear();
    S.reserve(length);
    using range = pair<typename vector<T>::const_iterator, typename vector<T>::const_iterator>;
    auto cmp = [](range a, range b) { return *(a.first) > *(b.first); };
    priority_queue<range, vector<range>, decltype(cmp)> PQ(cmp);
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i].begin() != v[i].end()) {
            PQ.push({v[i].begin(), v[i].end()});
        }

        while (!PQ.empty()) {
            auto R = PQ.top();
            PQ.pop();
            S.push_back(*(R.first));
            R.first = R.first + 1;
            if (R.first != R.second) {
                PQ.push(R);
            }
        }
    }
}

int main() {
    vector<vector<int>> A = {{1, 2, 4}, {}, {2, 4, 6}, {3, 6, 8, 9}};
    vector<int> B;
    merge(A, B);

    for (const auto& x: B) {
      cout << x << endl;
    }

    cin.get();
    cout << '\n' << "Press any key to continue...";
    cin.get();
    return 0;
}
