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

#include <string>

#include "chess/bitboard.h"
#include "chess/move.h"
#include "utils/hashcat.h"

namespace cczero {

struct MoveExecution;

// Represents a board position.
// Unlike most chess engines, the board is mirrored for black.
class ChessBoard {
   public:
    static const std::string kStartingFen;

    // Sets position from FEN string.
    // If @no_capture_ply and @moves are not nullptr, they are filled with
    // number of moves without capture and number of full moves since the
    // beginning of the game.
    void SetFromFen(const std::string& fen, int* no_capture_ply = nullptr,
                    int* moves = nullptr);
    // Nullifies the whole structure.
    void Clear();
    // Swaps black and white pieces and mirrors them relative to the
    // middle of the board. (what was on file 1 appears on file 8, what was
    // on rank b remains on b).
    void Mirror();

    // Generates list of possible moves for "ours" (white), but may leave king
    // under check.
    MoveList GeneratePseudolegalMoves() const;
    // Applies the move. (Only for "ours" (white)). Returns true if 50 moves
    // counter should be removed.
    bool ApplyMove(Move move);
    // Checks whether at least one of the sides has mating material.
    bool HasMatingMaterial() const;
    // Generates legal moves.
    MoveList GenerateLegalMoves() const;
    // Check whether pseudolegal move is legal.
    bool IsLegalMove(Move move) const;
    // Returns a list of legal moves and board positions after the move is made.
    std::vector<MoveExecution> GenerateLegalMovesAndPositions() const;

    uint64_t Hash() const {
        return HashCat({our_pieces_.as_int(), their_pieces_.as_int(),
                        rooks_.as_int(), knights_.as_int(), bishops_.as_int(),
                        advisors_.as_int(), cannons_.as_int(), pawns_.as_int(),
                        our_king_.as_int(), their_king_.as_int(), flipped_});
    }

    std::string DebugString() const;

    BitBoard ours() const { return our_pieces_; }
    BitBoard theirs() const { return their_pieces_; }
    BitBoard rooks() const { return rooks_; }
    BitBoard knights() const { return knights_; }
    BitBoard bishops() const { return bishops_; }
    BitBoard advisors() const { return advisors_; }
    BitBoard cannons() const { return cannons_; }
    BitBoard pawns() const { return pawns_; }
    BitBoard our_king() const { return 1ull << our_king_.as_int(); }
    BitBoard their_king() const { return 1ull << their_king_.as_int(); }
    bool flipped() const { return flipped_; }

    bool operator==(const ChessBoard& other) const {
        return (our_pieces_ == other.our_pieces_) &&
               (their_pieces_ == other.their_pieces_) &&
               (rooks_ == other.rooks_) && (bishops_ == other.bishops_) &&
               (knights_ == other.knights_) && (advisors_ == other.advisors_) &&
               (cannons_ == other.cannons_) && (our_king_ == other.our_king_) &&
               (their_king_ == other.their_king_) &&
               (flipped_ == other.flipped_);
    }

    bool operator!=(const ChessBoard& other) const {
        return !operator==(other);
    }

   private:
    // All white pieces.
    BitBoard our_pieces_;
    // All black pieces.
    BitBoard their_pieces_;
    // Rooks.
    BitBoard rooks_;
    // Knights.
    BitBoard knights_;
    // Bishops;
    BitBoard bishops_;
    // Advisors.
    BitBoard advisors_;
    // Cannons.
    BitBoard cannons_;
    // Pawns.
    BitBoard pawns_;
    BoardSquare our_king_;
    BoardSquare their_king_;
    bool flipped_ = false;  // aka "Black to move".
};

// Stores the move and state of the board after the move is done.
struct MoveExecution {
    Move move;
    ChessBoard board;
    bool reset_50_moves;
};

}  // namespace cczero