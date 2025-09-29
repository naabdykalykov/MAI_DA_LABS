#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

struct TextData {
    std::vector<std::string> words;
    std::vector<std::pair<int, int>> coordinates;
};

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

std::vector<int> calculateZ(const std::vector<std::string>& s) {
    int n = s.size();
    if (n == 0) {
        return {};
    }
    std::vector<int> z(n);
    int l = 0, r = 0;
    for (int i = 1; i < n; ++i) {
        if (i <= r) {
            z[i] = std::min(r - i + 1, z[i - l]);
        }
        while (i + z[i] < n && s[z[i]] == s[i + z[i]]) {
            z[i]++;
        }
        if (i + z[i] - 1 > r) {
            l = i;
            r = i + z[i] - 1;
        }
    }
    return z;
}

std::vector<std::string> readPattern() {
    std::vector<std::string> patternWords;
    std::string line;
    if (std::getline(std::cin, line)) {
        std::stringstream pss(line);
        std::string word;
        while (pss >> word) {
            patternWords.push_back(toLower(word));
        }
    }
    return patternWords;
}


TextData readText() {
    TextData textData;
    std::string line;
    int lineNumber = 1;
    while (std::getline(std::cin, line)) {
        std::stringstream tss(line);
        std::string word;
        int wordNumber = 1;
        while (tss >> word) {
            textData.words.push_back(toLower(word));
            textData.coordinates.push_back({lineNumber, wordNumber});
            wordNumber++;
        }
        lineNumber++;
    }
    return textData;
}


void findAndPrintMatches(const std::vector<int>& zValues, int patternLength, const std::vector<std::pair<int, int>>& coordinates) {
    for (int i = patternLength + 1; i < zValues.size(); ++i) {
        if (zValues[i] == patternLength) {
            int textIndex = i - (patternLength + 1);
            const auto& coords = coordinates[textIndex];
            std::cout << coords.first << "," << coords.second << "\n";
        }
    }
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    std::vector<std::string> patternWords = readPattern();
    TextData textData = readText();

    if (patternWords.empty() || textData.words.empty()) {
        return 0;
    }


    int patternLen = patternWords.size();
    std::vector<std::string> combined = patternWords;
    combined.push_back("$");
    combined.insert(combined.end(), textData.words.begin(), textData.words.end());


    std::vector<int> z = calculateZ(combined);

    findAndPrintMatches(z, patternLen, textData.coordinates);

    return 0;
}