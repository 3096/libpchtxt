/**
 * @file pchtxt.cpp
 * @brief Parser for the Patch Text format
 * @author 3096
 *
 * Copyright (c) 2020 3096
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pchtxt.hpp"

namespace pchtxt {

// CONSTANTS

constexpr auto COMMENT_IDENTIFIER = "/";
constexpr auto ECHO_IDENTIFIER = "#";
// meta tags
constexpr auto TITLE_TAG = "@title";
constexpr auto PROGRAM_ID_TAG = "@program";
constexpr auto URL_TAG = "@url";
constexpr auto NSOBID_TAG = "@nsobid";  // legacy
// parsing tags
constexpr auto ENABLED_TAG = "@enabled";
constexpr auto DISABLED_TAG = "@disabled";
constexpr auto HEAP_TAG = "@heap";
constexpr auto STOP_PARSING_TAG = "@stop";
constexpr auto FLAG_TAG = "@flag";
// flags
constexpr auto BIG_ENDIAN_FLAG = "be";
constexpr auto LITTLE_ENDIAN_FLAG = "le";
constexpr auto NSOBID_FLAG = "nsobid";
constexpr auto NROBID_FLAG = "nrobid";
constexpr auto OFFSET_SHIFT_FLAG = "offset_shift";
constexpr auto PRINT_VALUES_FLAG = "print_values";

// utils

inline auto isStartsWith(std::string& checkedStr, std::string& targetStr) {
    return checkedStr.size() >= targetStr.size() and checkedStr.substr(0, targetStr.size()) == targetStr;
}

inline auto readLine(std::istream& in) -> std::string {
    auto line = std::string{};
    std::getline(in, line, '\n');
    return line;
}

inline void ltrim(std::string& str) {
    str.erase(begin(str), std::find_if(begin(str), end(str), [](char ch) { return !std::isspace(ch); }));
}

inline void rtrim(std::string& str) {
    str.erase(std::find_if(rbegin(str), rend(str), [](char ch) { return !std::isspace(ch); }).base(), end(str));
}

inline void trim(std::string& str) {
    ltrim(str);
    rtrim(str);
}

inline auto firstToken(std::string& str) {
    return std::string(begin(str), std::find_if(begin(str), end(str), [](char ch) { return std::isspace(ch); }));
}

inline auto commentPos(std::string& str) {
    auto pos = 0;
    auto isInString = false;
    for (auto ch : str) {
        if (not isInString and ch == COMMENT_IDENTIFIER[0]) {
            break;
        }
        if (ch == '"') {
            isInString = !isInString;
        }
        pos++;
    }
    return pos;
}

inline auto lineIsComment(std::string& str) { return commentPos(str) == 0; }

inline auto getLineComment(std::string& str) {
    auto result = str.substr(commentPos(str));
    trim(result);
    return result;
}

inline void trimComment(std::string& str) {
    str.erase(commentPos(str));
    trim(str);
}

// not utils

auto parsePchtxt(std::istream& input) -> PatchTextOutput {
    auto throwAwaySs = std::stringstream{};
    return parsePchtxt(input, throwAwaySs);
}

auto parsePchtxt(std::istream& input, std::stringstream& logSs) -> PatchTextOutput {
    auto result = PatchTextOutput{};
    result.meta = getPchtxtMeta(input, logSs);
    // TODO:
    return result;
}

auto getPchtxtMeta(std::istream& input) -> PatchTextMeta {
    auto throwAwaySs = std::stringstream{};
    return getPchtxtMeta(input, throwAwaySs);
}

auto getPchtxtMeta(std::istream& input, std::stringstream& logSs) -> PatchTextMeta {
    auto result = PatchTextMeta{};
    auto tagToValueMap = std::unordered_map<std::string, std::string&>{
        {"@title", result.title}, {"@program", result.programId}, {"@url", result.url}};

    auto legacyTitle = std::string{};

    auto curLineNum = 1;
    while (true) {
        auto line = readLine(input);
        if (line.empty()) {
            logSs << "meta parsing reached end of file" << std::endl;
            break;
        }

        // meta should stops at an empty line
        trim(line);
        if (line.empty()) {
            logSs << "L" << curLineNum << ": done parsing meta" << std::endl;
            break;
        }

        trimComment(line);

        if (line[0] == '@') {
            auto curTag = firstToken(line);
            if (curTag == STOP_PARSING_TAG) break;

            auto curTagFound = tagToValueMap.find(curTag);
            if (curTagFound != end(tagToValueMap)) {
                auto curTagValue = line.substr(curTag.size());
                trim(curTagValue);
                // strip quatation marks if necessary
                if (curTagValue[0] == '"' and curTagValue[curTagValue.size() - 1] == '"') {
                    curTagValue = curTagValue.substr(1, curTagValue.size() - 2);
                }
                curTagFound->second = curTagValue;
                logSs << "L" << curLineNum << ": meta: " << curTag << "=" << curTagValue << std::endl;
            }
        } else if (line[0] == '#') {
            legacyTitle = line.substr(1);
            trim(legacyTitle);
            logSs << "L" << curLineNum << ": found legacy style title: " << legacyTitle << std::endl;
        }

        curLineNum++;
    }

    if (result.title.empty()) {
        result.title = legacyTitle;
        logSs << "legacy style title used" << std::endl;
    }

    return result;
}

}  // namespace pchtxt
