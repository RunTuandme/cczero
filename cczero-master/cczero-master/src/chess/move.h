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

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include "chess/bitboard.h"
#include "utils/bititer.h"

namespace cczero {

class Move {
   public:
    Move() = default;
    Move(BoardSquare from, BoardSquare to)
        : data_(to.as_int() + (from.as_int() << 8)) {}
    Move(const std::string& str, bool black = false);
    Move(const char* str, bool black = false) : Move(std::string(str), black) {}

    BoardSquare to() const { return BoardSquare((uint8_t)data_); }
    BoardSquare from() const { return BoardSquare((uint8_t)(data_ >> 8)); }

    void SetTo(BoardSquare to) { data_ = (data_ & 0xFF00) | to.as_int(); }
    void SetFrom(BoardSquare from) {
        data_ = (data_ & 0xFF) | (from.as_int() << 8);
    }
    // 0 .. 16384, knight promotion and no promotion is the same.
    uint16_t as_packed_int() const;

    // 0 .. 2086, to use in neural networks.
    uint16_t as_nn_index() const;

    bool operator==(const Move& other) const { return data_ == other.data_; }

    bool operator!=(const Move& other) const { return !operator==(other); }
    operator bool() const { return data_ != 0; }

    void Mirror() {
        data_ = (data_ & 0xFF00) | (89 - (uint8_t)data_);
        data_ = (data_ & 0xFF) | ((89 - (uint8_t)(data_ >> 8)) << 8);
    }

    std::string as_string() const {
        std::string res = from().as_string() + to().as_string();
        return res;
    }

   private:
    // Move, using the following encoding:
    // bits 0..7 "to"-square
    // bits 8..16 "from"-square
    uint16_t data_ = 0;
};

using MoveList = std::vector<Move>;

}  // namespace cczero
