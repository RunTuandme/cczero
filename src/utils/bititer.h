/*
  This file is part of Chinese Chess Zero.
  Copyright (C) 2018 The CCZero Authors

  Chinese Chess Zero is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Chinese Chess Zero is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Chinese Chess Zero.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <immintrin.h>
#include <cstdint>

namespace cczero {

// Iterates over all set bits of the value, lower to upper. The value of
// dereferenced iterator is bit number (lower to upper, 0 based)
template <typename T>
class BitIterator {
   public:
    BitIterator(__uint128_t value) : value_(value){};
    bool operator!=(const BitIterator& other) { return value_ != other.value_; }

    void operator++() { value_ &= (value_ - 1); }
    T operator*() const {
        std::uint64_t hi = value_ >> 64;
        std::uint64_t lo = value_;
        int retval[3] = {_tzcnt_u64(lo), _tzcnt_u64(hi) + 64, 128};
        int idx = !lo + ((!lo) & (!hi));
        return retval[idx];
    }

   private:
    __uint128_t value_;
};

class IterateBits {
   public:
    IterateBits(__uint128_t value) : value_(value) {}
    BitIterator<int> begin() { return value_; }
    BitIterator<int> end() { return 0; }

   private:
    __uint128_t value_;
};

}  // namespace cczero