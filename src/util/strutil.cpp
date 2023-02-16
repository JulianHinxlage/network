//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include "strutil.h"

std::vector<std::string> strSplit(const std::string& string, const std::string& delimiter, bool includeEmpty) {
    std::vector<std::string> parts;
    std::string token;
    int delimiterIndex = 0;
    for (char c : string) {
        if ((int)delimiter.size() == 0) {
            parts.push_back({ c, 1 });
        }
        else if (c == delimiter[delimiterIndex]) {
            delimiterIndex++;
            if (delimiterIndex == delimiter.size()) {
                if (includeEmpty || (int)token.size() != 0) {
                    parts.push_back(token);
                }
                token.clear();
                delimiterIndex = 0;
            }
        }
        else {
            token += delimiter.substr(0, delimiterIndex);
            token.push_back(c);
            delimiterIndex = 0;
        }
    }
    token += delimiter.substr(0, delimiterIndex);
    if (includeEmpty || (int)token.size() != 0) {
        parts.push_back(token);
    }
    return parts;
}

std::vector<std::string> strSplit(const std::vector<std::string>& strings, const std::string& delimiter, bool includeEmpty) {
    std::vector<std::string> parts;
    for (auto& s : strings) {
        for (auto& i : strSplit(s, delimiter, includeEmpty)) {
            parts.push_back(i);
        }
    }
    return parts;
}

std::string strJoin(const std::vector<std::string>& strings, const std::string& delimiter) {
    std::string result;
    for (int i = 0; i < strings.size(); i++) {
        result += strings[i];
        if (i != strings.size() - 1) {
            result += delimiter;
        }
    }
    return result;
}

std::string strReplace(const std::string& string, const std::string& search, const std::string& replacement) {
    return strJoin(strSplit(string, search), replacement);
}

std::string strToLower(const std::string& str) {
    std::string result;
    for (char c : str) {
        result.push_back(std::tolower(c));
    }
    return result;
}

std::string strToUpper(const std::string& str) {
    std::string result;
    for (char c : str) {
        result.push_back(std::toupper(c));
    }
    return result;
}
