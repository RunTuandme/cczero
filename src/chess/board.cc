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

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "chess/board.h"
#include "utils/exception.h"

namespace cczero {

using std::string;

const string ChessBoard::kStartingFen =
    "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1";

void ChessBoard::Clear() {
    std::memset(reinterpret_cast<void*>(this), 0, sizeof(ChessBoard));
}

void ChessBoard::Mirror() {
    our_pieces_.Mirror();
    their_pieces_.Mirror();
    std::swap(our_pieces_, their_pieces_);
    rooks_.Mirror();
    knights_.Mirror();
    bishops_.Mirror();
    advisors_.Mirror();
    cannons_.Mirror();
    pawns_.Mirror();
    our_king_.Mirror();
    their_king_.Mirror();
    std::swap(our_king_, their_king_);
    flipped_ = !flipped_;
}

namespace {
static const BitBoard kPawnMoves[] = {
  {1, 0}, {0,  1}, {0, -1}}
  
static const BitBoard kKingMoves[] = {
	{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

static const BitBoard kBishopMoves[] = {
	{2, 2}, {2, -2}, {-2, 2}, {-2, -2}};

static const std::pair<int, int> kKnightMoves[] = {
	{1, 2}, {-1, 2}, {1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}, {-2, -1}};
  
static const std::pair<int, int> kMandarinMoves[] = {
	{1, 1}, {-1, 1}, {1, -1}, {-1, -1}}; 
  
// include kCannonDirection
static const std::pair<int, int> kRookDirections[] = {
	{1, 0}, {-1, 0}, {0, 1}, {0, -1}}; 
}  // namespace

MoveList ChessBoard::GeneratePseudolegalMoves() const {
    MoveList result;
    // King
    for (const auto& delta : kKingMoves) {
        const auto dst_row = our_king_.row() + delta.first;
        const auto dst_col = our_king_.col() + delta.second;
        if (!BoardSquare::IsValid(dst_row, dst_col)) continue;
        if (dst_col == their_king_.col()) {
            bool face = true;
            for (int count_row = 0; count_row <= 9 ; count_row++) {
                const BoardSquare block(count_row, dst_col);
                if (count_row > dst_row && count_row < their_king_.row()) {
                    if(our_pieces_.get(block) || their_king_.get(block)) {face = false;}
                }
            }
            if (face = true) continue;
        } // Judge face
        if (dst_row > 2 || dst_col < 3 || dst_col > 5) continue;
        const BoardSquare destination(dst_row, dst_col);
        if (our_pieces_.get(destination)) continue;
        if (IsUnderAttack(destination)) continue;
        result.emplace_back(our_king_, destination);
    }

    for (auto source : our_pieces_ - our_king_) {
        bool processed_piece = false;
        // Rook
        if (rooks_.get(source)) {
            processed_piece = true;
            for (const auto& direction : kRookDirections) {
                auto dst_row = source.row();
                auto dst_col = source.col();
                while (true) {
                    dst_row += direction.first;
                    dst_col += direction.second;
                    if (!BoardSquare::IsValid(dst_row, dst_col)) break;
                    const BoardSquare destination(dst_row, dst_col);
                    if (our_pieces_.get(destination)) break;
                    result.emplace_back(source, destination);
                    if (their_pieces_.get(destination)) break;
                }
            }
        }
        // Bishop
        if (bishops_.get(source)) {
            processed_piece = true;
            for (const auto& delta : kBishopMoves) {
                const auto dst_row = source.row() + delta.first;
                const auto dst_col = source.col() + delta.second; 
                const BoardSquare block(delta.first/2, delta.second/2);
                if (our_pieces_.get(block) || their_pieces_.get(block)) continue;
                if (!BoardSquare::IsValid(dst_row, dst_col)) continue;
                if (dst_row > 4) continue;
                const BoardSquare destination(dst_row, dst_col);
                if (our_pieces_.get(destination)) continue;
                result.emplace_back(source, destination);
                }
        }
        if (processed_piece) continue;
        // Pawns
        if (pawns_.get(source)) {
            for (const auto& delta : kPawnMoves) {
                const auto dst_row = source.row() + delta.first;
                const auto dst_col = source.col() + delta.second; 
                if (source.row() < 5 && delta.second != 0) continue;
                if (!BoardSquare::IsValid(dst_row, dst_col)) continue;
                const BoardSquare destination(dst_row, dst_col);
                if (our_pieces_.get(destination)) continue;
                result.emplace_back(source, destination);
                }
            }
        }
        // Knight
        if (knights_.get(source)){
            for (const auto& delta : kKnightMoves) {
                const auto dst_row = source.row() + delta.first;
                const auto dst_col = source.col() + delta.second;
                const BoardSquare block(delta.first/2, delta.second/2);
                if (our_pieces_.get(block) || their_pieces_.get(block)) continue;
                if (!BoardSquare::IsValid(dst_row, dst_col)) continue;
                const BoardSquare destination(dst_row, dst_col);
                if (our_pieces_.get(destination)) continue;
                result.emplace_back(source, destination);
                }
            }
        }
        // Cannon
        if (Cannons_.get(source)){
            for (const auto& direction : kRookDirections) {
                auto dst_row = source.row();
                auto dst_col = source.col();
                int count_block = 0;
                while (true) {
                    dst_row += direction.first;
                    dst_col += direction.second;
                    if (!BoardSquare::IsValid(dst_row, dst_col)) break;
                    const BoardSquare destination(dst_row, dst_col);
                    if (our_pieces_.get(destination) || their_pieces_.get(destination)) count_block++;
                    if (count_block == 1) continue;
                    if (count_block == 2) {
                        if(!their_pieces_.get(destination)) break;
                    }
                    result.emplace_back(source, destination);
                    if (count_block == 2) break;
                }
            }
        }
        // Mandarin
        if (Mandarins_get(source)){
            for (const auto& delta : kMandarinMoves) {
                const auto dst_row = source.row() + delta.first;
                const auto dst_col = source.col() + delta.second; 
                if (dst_row > 2 || dst_col < 3 || dst_col > 5) continue;
                if (!BoardSquare::IsValid(dst_row, dst_col)) continue;
                const BoardSquare destination(dst_row, dst_col);
                if (our_pieces_.get(destination)) continue;
                result.emplace_back(source, destination);
            }
        }
    }
    return result;
}

bool ChessBoard::ApplyMove(Move move) {
    const auto& from = move.from();
    const auto& to = move.to();
    const auto from_row = from.row();
    const auto from_col = from.col();
    const auto to_row = to.row();
    const auto to_col = to.col();

    // Move in our pieces.
    our_pieces_.reset(from);
    our_pieces_.set(to);

    // Remove captured piece
    bool reset_50_moves = their_pieces_.get(to);
    their_pieces_.reset(to);
    rooks_.reset(to);
    bishops_.reset(to);
    pawns_.reset(to);

    // If pawn was moved, reset 50 move draw counter.
    reset_50_moves |= pawns_.get(from);

    // King
    if (from == our_king_) {
        our_king_ = to;
        // Castling
        if (to_col - from_col > 1) {
            // 0-0
            our_pieces_.reset(7);
            rooks_.reset(7);
            our_pieces_.set(5);
            rooks_.set(5);
        } else if (from_col - to_col > 1) {
            // 0-0-0
            our_pieces_.reset(0);
            rooks_.reset(0);
            our_pieces_.set(3);
            rooks_.set(3);
        }
        return reset_50_moves;
    }

    // Now destination square for our piece is known.
    our_pieces_.set(to);


    // Ordinary move.
    rooks_.set_if(to, rooks_.get(from));
    bishops_.set_if(to, bishops_.get(from));
    pawns_.set_if(to, pawns_.get(from));
    rooks_.reset(from);
    bishops_.reset(from);
    pawns_.reset(from);

    // Set en passant flag.
    if (to_row - from_row == 2 && pawns_.get(to)) {
        pawns_.set(0, to_col);
    }
    return reset_50_moves;
}

bool ChessBoard::IsUnderAttack(BoardSquare square) const {
    const int row = square.row();
    const int col = square.col();
    // Check king
    {
        if (col == kcol) {
            bool face = true;
            const int krow = their_king_.row();
            const int kcol = their_king_.col();
            for (int count_row = 0; count_row <= 9 ; count_row++) {
                const BoardSquare block(count_row, col);
                if (count_row > row && count_row < krow) {
                    if(our_pieces_.get(block) || their_pieces_.get(block)) {face = false;}
                }
            }
            if (face = true) return true;
        }
    }
    // Check Rooks
    for (const auto& direction : kRookDirections) {
        auto dst_row = row;
        auto dst_col = col;
        while (true) {
            dst_row += direction.first;
            dst_col += direction.second;
            if (!BoardSquare::IsValid(dst_row, dst_col)) break;
            const BoardSquare destination(dst_row, dst_col);
            if (our_pieces_.get(destination)) break;
            if (their_pieces_.get(destination)) {
                if (rooks_.get(destination)) return true;
                break;
            }
        }
    }
    // Check pawns
    for (const auto& delta : kPawnMoves){
        auto dst_row = row + delta.first;
        auto dst_col = col + delta.second; 
        const BoardSquare destination(dst_row, dst_col);
        if (pawns_.get(destination)) return true;
    }
    // Check knights
    for (const auto& delta : kKnightMoves){
        auto dst_row = row + delta.first;
        auto dst_col = col + delta.second;
        if (!BoardSquare::IsValid(dst_row, dst_col)) continue;
        const BoardSquare destination(dst_row, dst_col);
        if (knights_.get(destination)) return true;
    }
    return false;
}

bool ChessBoard::IsLegalMove(Move move) const {
    const auto& from = move.from();
    const auto& to = move.to();

    ChessBoard board(*this);
    board.ApplyMove(move);
    if (board.IsUnderAttack(board.our_king_)) return false;

    // If it's kings move, check that destination
    // is not under attack.
    if (from == our_king_) {
        return true;
    }

    // Not check that piece was pinned. And it was, check that after the move
    // it is still on like of attack.
    int dx = from.col() - our_king_.col();
    int dy = from.row() - our_king_.row();

    // If it's not on the same file/rank/diagonal as our king, cannot be pinned.
    if (dx != 0 && dy != 0 && std::abs(dx) != std::abs(dy)) return true;
    dx = (dx > 0) - (dx < 0);  // Sign.
    dy = (dy > 0) - (dy < 0);
    auto col = our_king_.col();
    auto row = our_king_.row();
    while (true) {
        col += dx;
        row += dy;
        // Attacking line left board, good.
        if (!BoardSquare::IsValid(row, col)) return true;
        const BoardSquare square(row, col);
        // The source square of the move is now free.
        if (square == from) continue;
        // The destination square if the move is our piece. King is not under
        // attack.
        if (square == to) return true;
        // Our piece on the line. Not under attack.
        if (our_pieces_.get(square)) return true;
        if (their_pieces_.get(square)) {
            if (dx == 0 || dy == 0) {
                // Have to be afraid of rook-like piece.
                return !rooks_.get(square);
            } else {
                // Have to be afraid of bishop-like piece.
                return !bishops_.get(square);
            }
            return true;
        }
    }
}

MoveList ChessBoard::GenerateLegalMoves() const {
    MoveList move_list = GeneratePseudolegalMoves();
    MoveList result;
    result.reserve(move_list.size());

    for (Move m : move_list) {
        if (IsLegalMove(m)) result.emplace_back(m);
    }

    return result;
}

void ChessBoard::SetFromFen(const std::string& fen, int* no_capture_ply,
                            int* moves) {
    Clear();
    int row = 7;
    int col = 0;

    std::istringstream fen_str(fen);
    string board;
    string who_to_move;
    string castlings;
    string en_passant;
    int no_capture_halfmoves;
    int total_moves;
    fen_str >> board >> who_to_move >> castlings >> en_passant >>
        no_capture_halfmoves >> total_moves;

    if (!fen_str) throw Exception("Bad fen string: " + fen);

    for (char c : board) {
        if (c == '/') {
            --row;
            col = 0;
            continue;
        }
        if (std::isdigit(c)) {
            col += c - '0';
            continue;
        }

        if (std::isupper(c)) {
            // White piece.
            our_pieces_.set(row, col);
        } else {
            // Black piece.
            their_pieces_.set(row, col);
        }

        if (c == 'K') {
            our_king_.set(row, col);
        } else if (c == 'k') {
            their_king_.set(row, col);
        } else if (c == 'R' || c == 'r') {
            rooks_.set(row, col);
        } else if (c == 'B' || c == 'b') {
            bishops_.set(row, col);
        } else if (c == 'Q' || c == 'q') {
            rooks_.set(row, col);
            bishops_.set(row, col);
        } else if (c == 'P' || c == 'p') {
            pawns_.set(row, col);
        } else if (c == 'N' || c == 'n') {
            // Do nothing
        } else {
            throw Exception("Bad fen string: " + fen);
        }
        ++col;
    }

    if (who_to_move == "b" || who_to_move == "B") {
        Mirror();
    }
    if (no_capture_ply) *no_capture_ply = no_capture_halfmoves;
    if (moves) *moves = total_moves;
}

bool ChessBoard::HasMatingMaterial() const {
    if (!rooks_.empty() || !pawns_.empty()) {
        return true;
    }

#ifdef _MSC_VER
    int our = _mm_popcnt_u64(our_pieces_.as_int());
    int their = _mm_popcnt_u64(their_pieces_.as_int());
#else
    int our = __builtin_popcountll(our_pieces_.as_int());
    int their = __builtin_popcountll(their_pieces_.as_int());
#endif
    if (our + their < 4) {
        // K v K, K+B v K, K+N v K.
        return false;
    }
    if (!our_knights().empty() || !their_knights().empty()) {
        return true;
    }

    // Only kings and bishops remain.

    constexpr BitBoard kLightSquares(0x55AA55AA55AA55AAULL);
    constexpr BitBoard kDarkSquares(0xAA55AA55AA55AA55ULL);

    bool light_bishop = bishops_.intersects(kLightSquares);
    bool dark_bishop = bishops_.intersects(kDarkSquares);
    return light_bishop && dark_bishop;
}

string ChessBoard::DebugString() const {
    string result;
    for (int i = 7; i >= 0; --i) {
        for (int j = 0; j < 8; ++j) {
            if (!our_pieces_.get(i, j) && !their_pieces_.get(i, j)) {
                if (i == 2 && pawns_.get(0, j))
                    result += '*';
                else if (i == 5 && pawns_.get(7, j))
                    result += '*';
                else
                    result += '.';
                continue;
            }
            if (our_king_ == i * 8 + j) {
                result += 'K';
                continue;
            }
            if (their_king_ == i * 8 + j) {
                result += 'k';
                continue;
            }
            char c = '?';
            if ((pawns_ * kPawnMask).get(i, j)) {
                c = 'p';
            } else if (bishops_.get(i, j)) {
                if (rooks_.get(i, j))
                    c = 'q';
                else
                    c = 'b';
            } else if (rooks_.get(i, j)) {
                c = 'r';
            } else {
                c = 'n';
            }
            if (our_pieces_.get(i, j)) c = std::toupper(c);
            result += c;
        }
        if (i == 0) {
            result += " " + castlings_.as_string();
            result +=
                flipped_ ? " (from black's eyes)" : " (from white's eyes)";
            result += " Hash: " + std::to_string(Hash());
        }
        result += '\n';
    }
    return result;
}

}  // namespace cczero
