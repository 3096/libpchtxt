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

#include <set>

namespace pchtxt {

// CONSTANTS

constexpr auto COMMENT_IDENTIFIER = "/";
constexpr auto ECHO_IDENTIFIER = "#";
constexpr auto AUTHOR_IDENTIFIER_OPEN = "[";
constexpr auto AUTHOR_IDENTIFIER_CLOSE = "]";
constexpr auto AMS_CHEAT_IDENTIFIER_OPEN = "[";
constexpr auto AMS_CHEAT_IDENTIFIER_CLOSE = "]";
// meta tags
constexpr auto TITLE_TAG = "@title";
constexpr auto PROGRAM_ID_TAG = "@program";
constexpr auto URL_TAG = "@url";
constexpr auto NSOBID_TAG = "@nsobid";  // legacy
const auto META_TAGS = std::set<std::string_view>{TITLE_TAG, PROGRAM_ID_TAG, URL_TAG, NSOBID_TAG};
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
constexpr auto DEBUG_INFO_FLAG = "debug_info";
constexpr auto ALT_DEBUG_INFO_FLAG = "print_values";  // legacy

inline auto isStartsWith(std::string& checkedStr, std::string_view targetStr) {
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

inline auto getLineCommentContent(std::string& str) {
    auto commentContentStart = std::find_if(begin(str) + commentPos(str), end(str), [](char ch) {
        return not(std::isspace(ch) or ch == COMMENT_IDENTIFIER[0]);
    });
    auto result = std::string(commentContentStart, end(str));
    rtrim(result);
    return result;
}

inline auto getLineNoComment(std::string& str) {
    auto result = str.substr(0, commentPos(str));
    trim(result);
    return result;
}

// not utils

auto parsePchtxt(std::istream& input) -> PatchTextOutput {
    auto throwAwaySs = std::stringstream{};
    return parsePchtxt(input, throwAwaySs);
}

auto parsePchtxt(std::istream& input, std::ostream& logOs) -> PatchTextOutput {
    auto result = PatchTextOutput{};

    // parse meta
    auto curPos = input.tellg();
    result.meta = getPchtxtMeta(input, logOs);
    input.seekg(curPos);

    // parsing status
    auto curLineNum = 1;
    auto lastCommentLine = std::string{};
    auto curPatch = Patch{};
    auto curPatchCollection = PatchCollection{};
    // auto curBuildId = std::string{};
    // auto curTargetType = TargetType{};
    auto curOffsetShift = 0;
    auto curIsBigEndian = false;
    auto isAcceptingPatch = false;
    auto isAcceptingAmsCheat = false;
    auto stopParsing = false;
    auto logDebugInfo = false;

    while (true) {
        if (stopParsing) break;

        auto line = readLine(input);
        if (line.empty()) {
            logOs << "done parsing patches" << std::endl;
            break;
        }

        auto lineNoComment = getLineNoComment(line);

        switch (line[0]) {
            case '@': {  // tags
                if (isAcceptingAmsCheat) {
                    logOs << "L" << curLineNum << ": WARNING: AMS cheat [" << curPatch.name
                          << "] ended because parsing reached a tag" << std::endl;
                    if (not curPatch.contents.empty()) {
                        curPatchCollection.patches.push_back(curPatch);
                        logOs << "L" << curLineNum << ": AMS cheat read: " << curPatch.name << std::endl;
                    }
                    curPatch = Patch{};
                    isAcceptingAmsCheat = false;
                }

                auto curTag = firstToken(lineNoComment);

                if (curTag == STOP_PARSING_TAG) {  // stop parsing
                    logOs << "L" << curLineNum << ": done parsing patches (reached tag @stop)" << std::endl;
                    stopParsing = true;
                    break;

                } else if (curTag == ENABLED_TAG or curTag == DISABLED_TAG) {  // start of a new patch
                    if (curPatchCollection.buildId.empty()) {
                        logOs << "L" << curLineNum << ": ERROR: missing build id, abort parsing" << curTag
                              << std::endl;
                        stopParsing = true;
                        break;
                    }

                    if (not curPatch.contents.empty()) {
                        curPatchCollection.patches.push_back(curPatch);
                        logOs << "L" << curLineNum << ": patch read: " << curPatch.name << std::endl;
                    }
                    curPatch = Patch{};

                    if (curTag == ENABLED_TAG) {
                        curPatch.enabled = true;
                    }

                    // extract name and author
                    auto authorStartPos = lastCommentLine.find(AUTHOR_IDENTIFIER_OPEN);
                    auto authorEndPos = lastCommentLine.rfind(AUTHOR_IDENTIFIER_CLOSE);
                    auto patchName = lastCommentLine.substr(0, authorStartPos);
                    rtrim(patchName);
                    auto author = lastCommentLine.substr(authorStartPos + 1, authorEndPos - authorStartPos - 1);
                    trim(author);
                    curPatch.name = patchName;
                    curPatch.author = author;

                    isAcceptingPatch = true;

                    if (logDebugInfo) logOs << "L" << curLineNum << ": parsing patch: " << curPatch.name << std::endl;

                } else if (curTag == HEAP_TAG) {  // cur patch is heap patch
                    curPatch.type = HEAP;

                } else if (curTag == FLAG_TAG) {  // parse flag
                    auto flagContent = lineNoComment.substr(curTag.size() + 1);
                    ltrim(flagContent);
                    auto flagType = firstToken(flagContent);
                    ltrim(flagType);
                    auto flagValue = flagContent.substr(flagType.size());
                    trim(flagValue);

                    if (flagType == BIG_ENDIAN_FLAG) {
                        curIsBigEndian = true;

                    } else if (flagType == LITTLE_ENDIAN_FLAG) {
                        curIsBigEndian = false;

                    } else if (flagType == NSOBID_FLAG or flagType == NROBID_FLAG) {
                        if (not curPatch.contents.empty()) {
                            curPatchCollection.patches.push_back(curPatch);
                            logOs << "L" << curLineNum << ": patch read: " << curPatch.name << std::endl;
                        }
                        if (not curPatchCollection.patches.empty()) {
                            result.collections.push_back(curPatchCollection);
                            if (logDebugInfo)
                                logOs << "L" << curLineNum << ": parsing completed for " << curPatchCollection.buildId
                                      << std::endl;
                        }

                        curPatchCollection.buildId = flagValue;
                        if (flagType == NROBID_FLAG) {
                            curPatchCollection.targetType = NRO;
                        } else {
                            curPatchCollection.targetType = NSO;
                        }

                        isAcceptingPatch = false;

                        if (logDebugInfo)
                            logOs << "L" << curLineNum << ": parsing started for " << curPatchCollection.buildId
                                  << std::endl;

                    } else if (flagType == OFFSET_SHIFT_FLAG) {
                        // TODO:

                    } else if (flagType == DEBUG_INFO_FLAG or flagType == ALT_DEBUG_INFO_FLAG) {
                        logDebugInfo = true;
                        logOs << "L" << curLineNum << ": additional debug info enabled" << std::endl;

                    } else {
                        logOs << "L" << curLineNum << ": WARNING ignored unrecognized flag type: " << flagType
                              << std::endl;
                    }

                } else if (isStartsWith(line, NSOBID_TAG)) {  // legacy style nsobid
                    curPatchCollection.targetType = NSO;
                    curPatchCollection.buildId = lineNoComment.substr(std::string_view(NSOBID_TAG).size() + 1);
                    trim(curPatchCollection.buildId);

                    if (logDebugInfo)
                        logOs << "L" << curLineNum << ": parsing started for " << curPatchCollection.buildId
                              << " (legacy style bid)" << std::endl;

                } else if (META_TAGS.find(curTag) == end(META_TAGS)) {
                    logOs << "L" << curLineNum << ": WARNING ignored unrecognized tag: " << curTag << std::endl;
                }
                break;
            }

            case '#': {  // echo identifier
                logOs << "L" << curLineNum << ": " << line << std::endl;
                break;
            }

            case AMS_CHEAT_IDENTIFIER_OPEN[0]: {  // AMS cheat
                if (not curPatch.contents.empty()) {
                    curPatchCollection.patches.push_back(curPatch);
                    logOs << "L" << curLineNum << ": patch read: " << curPatch.name << std::endl;
                }

                curPatch = Patch{};
                auto amsCheatName = lineNoComment.substr(1, lineNoComment.rfind(AMS_CHEAT_IDENTIFIER_CLOSE) - 2);
                trim(amsCheatName);
                curPatch.name = amsCheatName;
                curPatch.author = std::string{};

                isAcceptingPatch = false;
                isAcceptingAmsCheat = true;
                break;
            }

            case '/': {  // comment identifier
                lastCommentLine = getLineCommentContent(line);
                break;
            }

            default: {
                if (isAcceptingPatch) {
                    // TODO: parse patch contents
                } else if (isAcceptingAmsCheat) {
                    trim(line);
                    if (line.empty()) {
                        if (not curPatch.contents.empty()) {
                            curPatchCollection.patches.push_back(curPatch);
                            logOs << "L" << curLineNum << ": AMS cheat read: " << curPatch.name << std::endl;
                        }
                        curPatch = Patch{};
                        isAcceptingAmsCheat = false;

                    } else {
                        curPatch.contents.push_back(
                            {0, std::vector<uint8_t>{begin(lineNoComment), end(lineNoComment)}});
                    }
                }
                break;
            }
        }

        curLineNum++;
    }

    // add last patch and collection
    if (not curPatch.contents.empty()) {
        curPatchCollection.patches.push_back(curPatch);
        logOs << "L" << curLineNum << ": patch read: " << curPatch.name << std::endl;
    }
    if (not curPatchCollection.patches.empty()) {
        result.collections.push_back(curPatchCollection);
        if (logDebugInfo)
            logOs << "L" << curLineNum << ": parsing completed for " << curPatchCollection.buildId << std::endl;
    }

    return result;
}

auto getPchtxtMeta(std::istream& input) -> PatchTextMeta {
    auto throwAwaySs = std::stringstream{};
    return getPchtxtMeta(input, throwAwaySs);
}

auto getPchtxtMeta(std::istream& input, std::ostream& logOs) -> PatchTextMeta {
    auto result = PatchTextMeta{};
    auto tagToValueMap = std::unordered_map<std::string_view, std::string&>{
        {TITLE_TAG, result.title}, {PROGRAM_ID_TAG, result.programId}, {URL_TAG, result.url}};

    auto legacyTitle = std::string{};

    auto curLineNum = 1;
    while (true) {
        auto line = readLine(input);
        if (line.empty()) {
            logOs << "meta parsing reached end of file" << std::endl;
            break;
        }

        // meta should stop at an empty line
        trim(line);
        if (line.empty()) {
            logOs << "L" << curLineNum << ": done parsing meta" << std::endl;
            break;
        }

        line = getLineNoComment(line);

        if (line[0] == '@') {
            auto curTag = firstToken(line);
            if (curTag == STOP_PARSING_TAG) {
                logOs << "done parsing meta (reached tag @stop)" << std::endl;
                break;
            }

            auto curTagFound = tagToValueMap.find(curTag);
            if (curTagFound != end(tagToValueMap)) {
                auto curTagValue = line.substr(curTag.size());
                trim(curTagValue);
                // strip quatation marks if necessary
                if (curTagValue[0] == '"' and curTagValue[curTagValue.size() - 1] == '"') {
                    curTagValue = curTagValue.substr(1, curTagValue.size() - 2);
                }
                curTagFound->second = curTagValue;
                logOs << "L" << curLineNum << ": meta: " << curTag << "=" << curTagValue << std::endl;
            }
        } else if (line[0] == '#') {  // echo identifier
            rtrim(line);
            logOs << "L" << curLineNum << ": " << line << std::endl;
            legacyTitle = line.substr(1);
            ltrim(legacyTitle);
        }

        curLineNum++;
    }

    if (result.title.empty()) {
        result.title = legacyTitle;
        logOs << "using \"" << legacyTitle << "\" as legacy style title" << std::endl;
    }

    return result;
}

}  // namespace pchtxt
