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

#include "chess/move.h"
#include "utils/exception.h"

namespace cczero {

namespace {

const Move kIdxToMove[] = {};

std::vector<unsigned short> BuildMoveIndices() {
    std::vector<unsigned short> res(4 * 64 * 64);
    for (size_t i = 0; i < sizeof(kIdxToMove) / sizeof(kIdxToMove[0]); ++i) {
        res[kIdxToMove[i].as_packed_int()] = i;
    }
    return res;
}

const std::vector<unsigned short> kMoveToIdx = BuildMoveIndices();
const int kKingCastleIndex =
    kMoveToIdx[BoardSquare("e1").as_int() * 64 + BoardSquare("h1").as_int()];
const int kQueenCastleIndex =
    kMoveToIdx[BoardSquare("e1").as_int() * 64 + BoardSquare("a1").as_int()];
}  // namespace

Move::Move(const std::string& str, bool black) {
    if (str.size() < 4) throw Exception("Bad move: " + str);
    SetFrom(BoardSquare(str.substr(0, 2), black));
    SetTo(BoardSquare(str.substr(2, 2), black));
}

uint16_t Move::as_packed_int() const {
        return static_cast<int>(promotion()) * 64 * 64 + from().as_int() * 64 +
               to().as_int();
}

uint16_t Move::as_nn_index() const {
    if (!castling()) return kMoveToIdx[as_packed_int()];
    if (from().col() < to().col()) return kKingCastleIndex;
    return kQueenCastleIndex;
}

}  // namespace cczero