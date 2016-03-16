#pragma once

// Modifications by Brandon Kentel
//
// Original algorithm:
//
// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <iterator>
#include <cstdint>

namespace boken {

class utf8_decoder_iterator : public std::iterator<std::forward_iterator_tag, uint32_t> {
    static constexpr uint32_t UTF8_ACCEPT {0};
    static constexpr uint32_t UTF8_REJECT {12};

    static constexpr uint8_t utf8d_[] {
      // The first part of the table maps bytes to character classes that
      // to reduce the size of the transition table and create bitmasks.
       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
       1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
       7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
       8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
      10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

      // The second part is a transition table that maps a combination
      // of a state of the automaton and a character class to a state.
       0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
      12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
      12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
      12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
      12,36,12,12,12,12,12,12,12,12,12,12,
    };
public:
    utf8_decoder_iterator() noexcept = default;
    explicit utf8_decoder_iterator(char const* const s) noexcept
      : s_ {s}
    {
        ++*this;
    }

    explicit operator bool() const noexcept {
        return s_ && (state_ != UTF8_REJECT);
    }

    utf8_decoder_iterator& operator++() noexcept {
        next_cp_();
        return *this;
    }

    uint32_t const& operator*() const noexcept {
        return codep_;
    }

    bool operator==(utf8_decoder_iterator const& other) const noexcept {
        return s_ == other.s_;
    }

    bool operator!=(utf8_decoder_iterator const& other) const noexcept {
        return !(*this == other);
    }
private:
    void next_byte_() noexcept {
        uint32_t const byte = static_cast<uint8_t>(*s_++);
        uint32_t const type = utf8d_[byte];

        codep_ = (state_ != UTF8_ACCEPT)
               ? (byte & 0x3Fu) | (codep_ << 6u)
               : (0xFFu >> type) & (byte);

        state_ = utf8d_[256u + state_ + type];
    }

    void next_cp_() noexcept {
        if (!s_ || !*s_) {
            s_ = nullptr;
            return;
        }

        while (state_ != UTF8_REJECT) {
            next_byte_();
            if (!*s_ || state_ == UTF8_ACCEPT) {
                break;
            }
        }
    }

    char const* s_     {};
    uint32_t    state_ {};
    uint32_t    codep_ {};
};

inline utf8_decoder_iterator begin(utf8_decoder_iterator d) noexcept {
    return d;
}

inline utf8_decoder_iterator end(utf8_decoder_iterator) noexcept {
    return {};
}

} //namespace boken
