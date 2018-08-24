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

#include "utils/bititer.h"

namespace cczero {

// Stores a coordinates of a single square.
class BoardSquare {
   public:
    constexpr BoardSquare() {}
    // As a single number, 0 to 90, bottom to top, left to right.
    // 0 is a1, 8 is b1, 63 is h8.
    constexpr BoardSquare(std::uint8_t num) : square_(num) {}
    // From row(bottom to top), and col(left to right), 0-based.
    constexpr BoardSquare(int row, int col) : BoardSquare(row * 9 + col) {}
    // From Square name, e.g e4. Only lowercase.
    BoardSquare(const std::string& str, bool black = false)
        : BoardSquare(black ? '9' - str[1] : str[1] - '0',
                      black ? 'h' - str[0] : str[0] - 'a') {}
    constexpr std::uint8_t as_int() const { return square_; }
    void set(int row, int col) { square_ = row * 9 + col; }

    // 0-based, bottom to top.
    int row() const { return square_ / 9; }
    // 0-based, left to right.
    int col() const { return square_ % 9; }

    void Mirror() { square_ = 89 - square_; }

    // Checks whether coordinates are within board.
    static bool IsValid(int row, int col) {
        return row >= 0 && col >= 0 && row < 10 && col < 9;
    }

    constexpr bool operator==(const BoardSquare& other) const {
        return square_ == other.square_;
    }

    constexpr bool operator!=(const BoardSquare& other) const {
        return square_ != other.square_;
    }

    // Returns the square in algebraic notation (e.g. "e4").
    std::string as_string() const {
        return std::string(1, 'a' + col()) + std::string(1, '0' + row());
    }

   private:
    std::uint8_t square_ = 0;
};

// Represents a board as an array of 90 bits.
// Bit enumeration goes from bottom to top, from left to right:
// Square a1 is bit 0, square a8 is bit 7, square b1 is bit 8.
class BitBoard {
   public:
    constexpr BitBoard(std::uint64_t high, std::uint64_t low) : high_(high), low_(low) {}
    constexpr BitBoard(__uint128_t board) : board_(board) {}
    BitBoard() = default;
    BitBoard(const BitBoard&) = default;

    __uint128_t as_int() const { return board_; }
    void clear() { board_ = 0; }

    // Sets the value for given square to 1 if cond is true.
    // Otherwise does nothing (doesn't reset!).
    void set_if(BoardSquare square, bool cond) {
        set_if(square.as_int(), cond);
    }
    void set_if(std::uint8_t pos, bool cond) {
        board_ |= (__uint128_t(cond) << pos);
    }
    void set_if(int row, int col, bool cond) {
        set_if(BoardSquare(row, col), cond);
    }

    // Sets value of given square to 1.
    void set(BoardSquare square) { set(square.as_int()); }
    void set(std::uint8_t pos) { board_ |= (__uint128_t(1) << pos); }
    void set(int row, int col) { set(BoardSquare(row, col)); }

    // Sets value of given square to 0.
    void reset(BoardSquare square) { reset(square.as_int()); }
    void reset(std::uint8_t pos) { board_ &= ~(__uint128_t(1) << pos); }
    void reset(int row, int col) { reset(BoardSquare(row, col)); }

    // Gets value of a square.
    bool get(BoardSquare square) const { return get(square.as_int()); }
    bool get(std::uint8_t pos) const { return board_ & (__uint128_t(1) << pos); }
    bool get(int row, int col) const { return get(BoardSquare(row, col)); }

    // Returns whether all bits of a board are set to 0.
    bool empty() const { return board_ == 0; }

    // Checks whether two bitboards have common bits set.
    bool intersects(const BitBoard& other) const {
        return board_ & other.board_;
    }

    // Flips black and white side of a board.

    void Mirror() {
        uint8_t s = 128;  // bit size; must be power of 2
        __uint128_t mask = ~(__uint128_t)0;
        while ((s >>= 1) > 0) {
            mask ^= (mask << s);
            board_ = ((board_ >> s) & mask) | ((board_ << s) & ~mask);
        }
        board_ >>= 38;
    }

    bool operator==(const BitBoard& other) const {
        return board_ == other.board_;
    }

    BitIterator<BoardSquare> begin() const { return board_; }
    BitIterator<BoardSquare> end() const { return 0; }

    std::string DebugString() const {
        std::string res;
        for (int i = 9; i >= 0; --i) {
            for (int j = 0; j < 9; ++j) {
                if (get(i, j))
                    res += '#';
                else
                    res += '.';
            }
            res += '\n';
        }
        return res;
    }

    // Applies a mask to the bitboard (intersects).
    BitBoard& operator*=(const BitBoard& a) {
        board_ &= a.board_;
        return *this;
    }

    friend void swap(BitBoard& a, BitBoard& b) {
        using std::swap;
        swap(a.board_, b.board_);
    }

    // Returns union (bitwise OR) of two boards.
    friend BitBoard operator+(const BitBoard& a, const BitBoard& b) {
        return {a.board_ | b.board_};
    }

    // Returns bitboard with one bit reset.
    friend BitBoard operator-(const BitBoard& a, const BoardSquare& b) {
        return {a.board_ & ~(__uint128_t(1) << b.as_int())};
    }

    // Returns difference (bitwise AND-NOT) of two boards.
    friend BitBoard operator-(const BitBoard& a, const BitBoard& b) {
        return {a.board_ & ~b.board_};
    }

    // Returns intersection (bitwise AND) of two boards.
    friend BitBoard operator*(const BitBoard& a, const BitBoard& b) {
        return {a.board_ & b.board_};
    }

   private:
    __uint128_t board_ = 0;
    std::uint64_t high_ = 0;
    std::uint64_t low_ = 0;
};

}  // namespace cczero
