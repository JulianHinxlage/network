//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <vector>
#include <string>

std::vector<std::string> strSplit(const std::string& string, const std::string& delimiter, bool includeEmpty = true);
std::vector<std::string> strSplit(const std::vector<std::string>& strings, const std::string& delimiter, bool includeEmpty = true);
std::string strJoin(const std::vector<std::string>& strings, const std::string& delimiter);
std::string strReplace(const std::string& string, const std::string& search, const std::string& replacement);
std::string strToLower(const std::string& str);
std::string strToUpper(const std::string& str);
