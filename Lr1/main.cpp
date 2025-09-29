#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>
#include <array>
#include <numeric>

using namespace std;

struct Pair {
    string key;
    string value;
};

int get_digit_value(char c) {
    if (isdigit(c)) {
        return c - '0';
    } else {
        return tolower(c) - 'a' + 10;
    }
}

vector<Pair> read_input() {
    vector<Pair> data;
    string line;
    while (getline(cin, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        Pair p;
        ss >> p.key;
        ss.ignore();
        getline(ss, p.value);
        data.push_back(p);
    }
    return data;
}

void counting_sort_pass_by_index(const vector<Pair>& data, vector<size_t>& indices, int digit_idx) {
    const int base = 16;
    size_t n = data.size();
    if (n == 0) return;

    vector<size_t> output_indices(n);
    array<int, base> count{};

    for (size_t i = 0; i < n; ++i) {
        size_t current_index = indices[i];
        int digit_value = get_digit_value(data[current_index].key[digit_idx]);
        count[digit_value]++;
    }

    for (int i = 1; i < base; ++i) {
        count[i] += count[i - 1];
    }

    for (int i = n - 1; i >= 0; --i) {
        size_t current_index = indices[i];
        int digit_value = get_digit_value(data[current_index].key[digit_idx]);
        output_indices[count[digit_value] - 1] = current_index;
        count[digit_value]--;
    }

    indices = output_indices;
}

void radix_sort_by_index(const vector<Pair>& data, vector<size_t>& indices) {
    const int key_length = 32;

    for (int digit_idx = key_length - 1; digit_idx >= 0; --digit_idx) {
        counting_sort_pass_by_index(data, indices, digit_idx);
    }
}

void print_output(const vector<Pair>& data, const vector<size_t>& indices) {
    for (const auto& index : indices) {
        cout << data[index].key << "\t" << data[index].value << endl;
    }
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    vector<Pair> data = read_input();
    if (data.empty()) return 0;

    vector<size_t> indices(data.size());
    iota(indices.begin(), indices.end(), 0);

    radix_sort_by_index(data, indices);

    print_output(data, indices);

    return 0;
}