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
    enum class Promotion : std::uint8_t { None, Queen, Rook, Bishop, Knight };
    Move() = default;
    Move(BoardSquare from, BoardSquare to)
        : data_(to.as_int() + (from.as_int() << 6)) {}
    Move(BoardSquare from, BoardSquare to, Promotion promotion)
        : data_(to.as_int() + (from.as_int() << 6) +
                (static_cast<uint8_t>(promotion) << 12)) {}
    Move(const std::string& str, bool black = false);
    Move(const char* str, bool black = false) : Move(std::string(str), black) {}

    BoardSquare to() const { return BoardSquare(data_ & kToMask); }
    BoardSquare from() const { return BoardSquare((data_ & kFromMask) >> 6); }
    Promotion promotion() const {
        return Promotion((data_ & kPromoMask) >> 12);
    }
    bool castling() const { return (data_ & kCastleMask) != 0; }
    void SetCastling() { data_ |= kCastleMask; }

    void SetTo(BoardSquare to) { data_ = (data_ & ~kToMask) | to.as_int(); }
    void SetFrom(BoardSquare from) {
        data_ = (data_ & ~kFromMask) | (from.as_int() << 6);
    }
    void SetPromotion(Promotion promotion) {
        data_ = (data_ & ~kPromoMask) | (static_cast<uint8_t>(promotion) << 12);
    }
    // 0 .. 16384, knight promotion and no promotion is the same.
    uint16_t as_packed_int() const;

    // 0 .. 1857, to use in neural networks.
    uint16_t as_nn_index() const;

    // We ignore the castling bit, because UCI's `position moves ...` commands
    // specify squares and promotions, but NOT whether or not a move is
    // castling. NodeTree::MakeMove and all Move::Move constructors are thus so
    // ignorant.
    bool operator==(const Move& other) const {
        return (data_ | kCastleMask) == (other.data_ | kCastleMask);
    }

    bool operator!=(const Move& other) const { return !operator==(other); }
    operator bool() const { return data_ != 0; }

    void Mirror() { data_ ^= 0b111000111000; }

    std::string as_string() const {
        std::string res = from().as_string() + to().as_string();
        switch (promotion()) {
            case Promotion::None:
                return res;
            case Promotion::Queen:
                return res + 'q';
            case Promotion::Rook:
                return res + 'r';
            case Promotion::Bishop:
                return res + 'b';
            case Promotion::Knight:
                return res + 'n';
        }
        assert(false);
        return "Error!";
    }

   private:
    uint16_t data_ = 0;
    // Move, using the following encoding:
    // bits 0..5 "to"-square
    // bits 6..11 "from"-square
    // bits 12..14 promotion value
    // bit 15 whether move is a castling

    enum Masks : uint16_t {
        kToMask = 0b0000000000111111,
        kFromMask = 0b0000111111000000,
        kPromoMask = 0b0111000000000000,
        kCastleMask = 0b1000000000000000,
    };
};

using MoveList = std::vector<Move>;

}  // namespace cczero
