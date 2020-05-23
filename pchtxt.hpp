/**
 * @file pchtxt.hpp
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

#pragma once

#include <functional>
#include <list>
#include <sstream>
#include <string>
#include <vector>

namespace pchtxt {

/**
 * The content patches
 */
struct PatchContent {
    uint32_t offset;            /*!< The offset to patch at */
    std::vector<uint8_t> value; /*!< The value to be patched, in bytes */
};

/**
 * Type of the Patch
 */
enum PatchType { BIN, HEAP };

/**
 * One patch in the output
 */
struct Patch {
    std::string name;                 /*!< Name of the patch */
    std::string author;               /*!< Author of the patch */
    bool enabled;                     /*!< The patch is currently enabled or not */
    PatchType type;                   /*!< The patch is currently enabled or not */
    std::list<PatchContent> contents; /*!< List of contents for the patch */
};

/**
 * Target type of the PatchCollection
 */
enum TargetType { NSO, NRO };

/**
 * Collection of patches for one binary file
 */
struct PatchCollection {
    std::string buildId;      /*!< Build ID of the target binary */
    TargetType targetType;    /*!< Type of the target binary */
    std::list<Patch> patches; /*!< List of patches to be applied */
};

struct PatchTextMeta {
    std::string title;     /*!< Title of the Patch Text for description purposes. For example: the game's name */
    std::string programId; /*!< Program ID, also know as Title ID */
    std::string url;       /*!< An url that can be used to update the pchtxt with */
};

/**
 * Compiled output for one Patch Text. Can contain outputs for multiple binaries
 */
struct PatchTextOutput {
    PatchTextMeta meta;                     /*!< Meta data for the Patch Text file */
    std::list<PatchCollection> collections; /*!< Patch collections, each collection is intended for one binary */
};

/**
 * Compile a complete output from one Patch Text
 * @param input an istream from the pchtxt file
 * @param logSs [optional] a streamstream to capture parsing logs
 * @return The PatchTextOutput struct containing all the parsed information from the Patch Text
 */
inline auto parsePchtxt(std::istream& input) -> PatchTextOutput;
auto parsePchtxt(std::istream& input, std::stringstream& logSs) -> PatchTextOutput;

/**
 * Parse the meta data for the Patch Text
 * @param input an istream from the pchtxt file
 * @param logSs [optional] a streamstream to capture parsing logs
 * @return The PatchTextMeta struct containing the meta information of the Patch Text
 */
inline auto getPchtxtMeta(std::istream& input) -> PatchTextMeta;
auto getPchtxtMeta(std::istream& input, std::stringstream& logSs) -> PatchTextMeta;

}  // namespace pchtxt
