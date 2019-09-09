/*
 * Copyright 2017-2019 aquenos GmbH.
 * Copyright 2017-2019 Karlsruhe Institute of Technology.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * This software has been developed by aquenos GmbH on behalf of the
 * Karlsruhe Institute of Technology's Institute for Beam Physics and
 * Technology.
 *
 * This software contains code originally developed by aquenos GmbH for
 * the s7nodave EPICS device support. aquenos GmbH has relicensed the
 * affected portions of code from the s7nodave EPICS device support
 * (originally licensed under the terms of the GNU GPL) under the terms
 * of the GNU LGPL version 3 or newer.
 */

#include <algorithm>
#include <cctype>
#include <climits>
#include <stdexcept>
#include <tuple>

#include "Open62541RecordAddress.h"

namespace open62541 {
namespace epics {

namespace {

const std::string emptyString;

bool compareStringsIgnoreCase(const std::string &str1,
    const std::string &str2) {
  if (str1.length() != str2.length()) {
    return false;
  }
  return std::equal(str1.begin(), str1.end(), str2.begin(),
      [](char c1, char c2) {return std::tolower(c1) == std::tolower(c2);});
}

std::pair<std::size_t, std::size_t> findNextToken(const std::string &str,
    const std::string &delimiters, std::size_t startPos) {
  if (str.length() == 0) {
    return std::make_pair(std::string::npos, 0);
  }
  std::size_t startOfToken = str.find_first_not_of(delimiters, startPos);
  if (startOfToken == std::string::npos) {
    return std::make_pair(std::string::npos, 0);
  }
  std::size_t endOfToken = str.find_first_of(delimiters, startOfToken);
  if (endOfToken == std::string::npos) {
    return std::make_pair(startOfToken, str.length() - startOfToken);
  }
  return std::make_pair(startOfToken, endOfToken - startOfToken);
}

bool startsWithIgnoreCase(const std::string &str1, const std::string &str2) {
  if (str1.length() < str2.length()) {
    return false;
  }
  return std::equal(str2.begin(), str2.end(), str1.begin(),
      [](char c1, char c2) {return std::tolower(c1) == std::tolower(c2);});
}

std::string trim(const std::string &str, const std::string &whitespace =
    " \t\n\v\f\r") {
  auto start = str.find_first_not_of(whitespace);
  if (start == std::string::npos) {
    return std::string();
  }
  auto end = str.find_last_not_of(whitespace) + 1;
  return str.substr(start, end - start);
}

UaNodeId parseNodeId(const std::string &nodeIdString) {
  if (startsWithIgnoreCase(nodeIdString, "num:")) {
    auto commaPos = nodeIdString.find(',');
    if (commaPos == std::string::npos) {
      throw std::invalid_argument(
          std::string("Invalid node ID in record address: ") + nodeIdString);
    }
    auto nsString = nodeIdString.substr(4, commaPos - 4);
    auto numString = nodeIdString.substr(commaPos + 1);
    unsigned long ns;
    try {
      std::size_t convertedLength;
      ns = std::stoul(nsString, &convertedLength);
      if (convertedLength != nsString.length()) {
        throw std::invalid_argument("Only partial string has been converted.");
      }
    } catch (std::invalid_argument&) {
      throw std::invalid_argument(
          std::string("Invalid namespace index in node ID: ") + nodeIdString);
    } catch (std::out_of_range&) {
      throw std::invalid_argument(
          std::string("Invalid namespace index in node ID: ") + nodeIdString);
    }
    if (ns > 65535) {
      throw std::invalid_argument(
          std::string("Invalid namespace index in node ID: ") + nodeIdString);
    }
    // We use the stoul function for converting. This only works, if the unsigned
    // long data-type is large enough (which it should be on virtually all
    // platforms).
    static_assert(sizeof(unsigned long) >= sizeof(std::uint32_t),
        "unsigned long data-type is not large enough to hold a uint32_t");
    unsigned long num;
    try {
      std::size_t convertedLength;
      num = std::stoul(numString, &convertedLength);
      if (convertedLength != numString.length()) {
        throw std::invalid_argument("Only partial string has been converted.");
      }
    } catch (std::invalid_argument&) {
      throw std::invalid_argument(
          std::string("Invalid numeric ID in node ID: ") + nodeIdString);
    } catch (std::out_of_range&) {
      throw std::invalid_argument(
          std::string("Invalid numeric ID in node ID: ") + nodeIdString);
    }
    return UaNodeId::createNumeric(static_cast<UA_UInt16>(ns),
        static_cast<UA_UInt32>(num));
  } else if (startsWithIgnoreCase(nodeIdString, "str:")) {
    auto commaPos = nodeIdString.find(',');
    if (commaPos == std::string::npos) {
      throw std::invalid_argument(
          std::string("Invalid node ID in record address: ") + nodeIdString);
    }
    auto nsString = nodeIdString.substr(4, commaPos - 4);
    auto idString = nodeIdString.substr(commaPos + 1);
    unsigned long ns;
    try {
      std::size_t convertedLength;
      ns = std::stoul(nsString, &convertedLength);
      if (convertedLength != nsString.length()) {
        throw std::invalid_argument("Only partial string has been converted.");
      }
    } catch (std::invalid_argument&) {
      throw std::invalid_argument(
          std::string("Invalid namespace index in node ID: ") + nodeIdString);
    } catch (std::out_of_range&) {
      throw std::invalid_argument(
          std::string("Invalid namespace index in node ID: ") + nodeIdString);
    }
    if (ns > 65535) {
      throw std::invalid_argument(
          std::string("Invalid namespace index in node ID: ") + nodeIdString);
    }
    return UaNodeId::createString(static_cast<UA_UInt16>(ns), idString);
  } else {
    throw std::invalid_argument(
        std::string("Invalid node ID in record address: ") + nodeIdString);
  }
}

}

Open62541RecordAddress::Open62541RecordAddress(
    const std::string &addressString) :
    conversionMode(ConversionMode::automatic), dataType(DataType::unspecified),
    readOnInit(true),
    samplingInterval(std::numeric_limits<double>::quiet_NaN()),
    subscription("default") {
  const std::string delimiters(" \t\n\v\f\r");
  std::size_t tokenStart, tokenLength;
  // First, read the device name.
  std::tie(tokenStart, tokenLength) = findNextToken(addressString, delimiters,
      0);
  if (tokenStart == std::string::npos) {
    throw std::invalid_argument(
        "Could not find connection ID in record address.");
  }
  // If there is an options string, the opening parenthesis might directly
  // follow the connection ID.
  std::size_t optionsStringStart = addressString.find('(');
  if (optionsStringStart != std::string::npos
      && optionsStringStart < tokenStart + tokenLength) {
    tokenLength = optionsStringStart - tokenStart;
    if (tokenLength == 0) {
      throw std::invalid_argument(
          "Could not find connection ID in record address.");
    }
  }
  this->connectionId = addressString.substr(tokenStart, tokenLength);
  // The next token can be extra options or the node ID. We can only tell by
  // inspecting the token.
  std::tie(tokenStart, tokenLength) = findNextToken(addressString, delimiters,
      tokenStart + tokenLength);
  if (tokenStart == std::string::npos) {
    throw std::invalid_argument("Could not find node ID in record address.");
  }
  // The optional options string starts with an opening parenthesis, a node ID
  // never does.
  if (addressString[tokenStart] == '(') {
    bool optionsStringComplete = false;
    std::string optionToken;
    for (auto i = tokenStart + 1; i < addressString.size(); ++i) {
      char c = addressString[i];
      if (c == ',' || c == ')') {
        optionToken = trim(optionToken, delimiters);
        if (compareStringsIgnoreCase(optionToken, "no_read_on_init")) {
          readOnInit = false;
        } else if (startsWithIgnoreCase(optionToken, "conversion_mode=")) {
          std::string optionValue = optionToken.substr(16);
          if (compareStringsIgnoreCase(optionValue, "convert")) {
            this->conversionMode = ConversionMode::convert;
          } else if (compareStringsIgnoreCase(optionValue, "direct")) {
            this->conversionMode = ConversionMode::direct;
          } else {
            throw std::invalid_argument(
                std::string("Unrecognized conversion mode in record address: ")
                    + optionValue);
          }
        } else if (startsWithIgnoreCase(optionToken, "sampling_interval=")) {
          std::string optionValue = optionToken.substr(18);
          try {
            std::size_t convertedLength;
            this->samplingInterval = std::stod(optionValue, &convertedLength);
            if (convertedLength != optionValue.length()) {
              throw std::invalid_argument("Only partial string has been converted.");
            }
          } catch (std::invalid_argument&) {
            throw std::invalid_argument(
              std::string("Invalid sampling_interval: ") + optionValue);
          }
        } else if (startsWithIgnoreCase(optionToken, "subscription=")) {
          std::string optionValue = optionToken.substr(13);
          this->subscription = optionValue;
        } else if (i != tokenStart + 1) {
          // An empty options token is only allowed if the whole options string
          // is empty.
          throw std::invalid_argument(
              std::string(
                  "Unrecognized token in options string of record address: ")
                  + optionToken);
        }
        optionToken.clear();
      } else {
        optionToken += c;
      }
      if (c == ')') {
        optionsStringComplete = true;
        tokenLength = i - tokenStart + 1;
        break;
      }
    }
    // The options string must be terminated by a closing parenthesis.
    if (!optionsStringComplete) {
      throw std::invalid_argument(
          "Unbalanced parentheses in options string of record address.");
    }
    // The next token is the node ID.
    std::tie(tokenStart, tokenLength) = findNextToken(addressString, delimiters,
        tokenStart + tokenLength);
  }
  // Next, read the node ID. However, we delay the parsing of the node ID until
  // we have read the rest of the string. This way, we do not have to free a
  // node ID data-structure when there is a problem later in the string.
  if (tokenStart == std::string::npos) {
    throw std::invalid_argument("Could not find node ID in record address.");
  }
  std::string nodeIdString;
  // If the node ID contains whitespace it must be escaped.
  bool lastCharWasEscape = false;
  bool nodeIdComplete = false;
  for (auto i = tokenStart; i < addressString.size() && !nodeIdComplete; ++i) {
    char c = addressString[i];
    switch (c) {
    case '\\':
      if (lastCharWasEscape) {
        nodeIdString += '\\';
        lastCharWasEscape = false;
      } else {
        lastCharWasEscape = true;
      }
      break;
    case ' ':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
      if (!lastCharWasEscape) {
        // End of node ID.
        tokenLength = i - tokenStart;
        nodeIdComplete = true;
      } else {
        nodeIdString += c;
        lastCharWasEscape = false;
      }
      break;
    default:
      if (lastCharWasEscape) {
        // If there is a backslash before any other character, we consider this
        // an error.
        throw std::invalid_argument("Unexpected escape sequence in node ID.");
      }
      nodeIdString += c;
    }
  }
  // If there is a trailing backslash at the end of the string, we treat it like
  // a backslash in front of a non-special character.
  if (lastCharWasEscape) {
    throw std::invalid_argument("Unexpected escape sequence in node ID.");
  }
  // If the nodeIdComplete flag is not set, it means that the node ID ends with
  // the end of the address string.
  if (!nodeIdComplete) {
    tokenLength = addressString.size() - tokenStart;
  }
  // Read the data type
  std::tie(tokenStart, tokenLength) = findNextToken(addressString, delimiters,
      tokenStart + tokenLength);
  if (tokenStart != std::string::npos) {
    std::string dataTypeString = addressString.substr(tokenStart, tokenLength);
    if (compareStringsIgnoreCase(dataTypeString, "boolean")) {
      dataType = DataType::boolean;
    } else if (compareStringsIgnoreCase(dataTypeString, "sbyte")) {
      dataType = DataType::sbyte;
    } else if (compareStringsIgnoreCase(dataTypeString, "byte")) {
      dataType = DataType::byte;
    } else if (compareStringsIgnoreCase(dataTypeString, "int16")) {
      dataType = DataType::int16;
    } else if (compareStringsIgnoreCase(dataTypeString, "uint16")) {
      dataType = DataType::uint16;
    } else if (compareStringsIgnoreCase(dataTypeString, "int32")) {
      dataType = DataType::int32;
    } else if (compareStringsIgnoreCase(dataTypeString, "uint32")) {
      dataType = DataType::uint32;
    } else if (compareStringsIgnoreCase(dataTypeString, "int64")) {
      dataType = DataType::int64;
    } else if (compareStringsIgnoreCase(dataTypeString, "uint64")) {
      dataType = DataType::uint64;
    } else if (compareStringsIgnoreCase(dataTypeString, "float")) {
      dataType = DataType::floatType;
    } else if (compareStringsIgnoreCase(dataTypeString, "double")) {
      dataType = DataType::doubleType;
    } else {
      throw std::invalid_argument(
          "Invalid data type in record address: " + dataTypeString);
    }
  }
  // There should be no more data after the data type.
  std::tie(tokenStart, tokenLength) = findNextToken(addressString, delimiters,
      tokenStart + tokenLength);
  if (tokenStart != std::string::npos) {
    throw std::invalid_argument(
        "Invalid trailing data at end of record address: "
            + addressString.substr(tokenStart, tokenLength));
  }
  // Finally, we parse the node ID.
  this->nodeId = parseNodeId(nodeIdString);
}

}
}
