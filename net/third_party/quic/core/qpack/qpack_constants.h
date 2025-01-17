// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_THIRD_PARTY_QUIC_CORE_QPACK_QPACK_CONSTANTS_H_
#define NET_THIRD_PARTY_QUIC_CORE_QPACK_QPACK_CONSTANTS_H_

#include <cstdint>

namespace quic {

// Wire format defined in
// https://quicwg.org/base-drafts/draft-ietf-quic-qpack.html#rfc.section.5.4.2.

// 5.4.2.1. Indexed Header Field
const uint8_t kIndexedHeaderFieldOpcode = 0b10000000;
const uint8_t kIndexedHeaderFieldOpcodeMask = 0b10000000;
const uint8_t kIndexedHeaderFieldPrefixLength = 6;

// 5.4.2.2. Indexed Header Field With Post-Base Index
const uint8_t kIndexedHeaderFieldPostBaseOpcode = 0b00010000;
const uint8_t kIndexedHeaderFieldPostBaseOpcodeMask = 0b11110000;
const uint8_t kIndexedHeaderFieldPostBasePrefixLength = 4;

// 5.4.2.3. Literal Header Field With Name Reference
const uint8_t kLiteralHeaderFieldNameReferenceOpcode = 0b01000000;
const uint8_t kLiteralHeaderFieldNameReferenceOpcodeMask = 0b11000000;
const uint8_t kLiteralHeaderFieldNameReferencePrefixLength = 4;

// 5.4.2.4. Literal Header Field With Post-Base Name Reference
const uint8_t kLiteralHeaderFieldPostBaseOpcode = 0b00000000;
const uint8_t kLiteralHeaderFieldPostBaseOpcodeMask = 0b11110000;
const uint8_t kLiteralHeaderFieldPostBasePrefixLength = 3;

// 5.4.2.5. Literal Header Field Without Name Reference
const uint8_t kLiteralHeaderFieldOpcode = 0b00100000;
const uint8_t kLiteralHeaderFieldOpcodeMask = 0b11100000;
const uint8_t kLiteralNameHuffmanMask = 0b00001000;
const uint8_t kLiteralHeaderFieldPrefixLength = 3;

// Value encoding for instructions with literal value.
const uint8_t kLiteralValueHuffmanMask = 0b10000000;
const uint8_t kLiteralValueWithoutHuffmanEncoding = 0b00000000;
const uint8_t kLiteralValuePrefixLength = 7;

}  // namespace quic

#endif  // NET_THIRD_PARTY_QUIC_CORE_QPACK_QPACK_CONSTANTS_H_
