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
static const BitBoard kKingMoves[] = {};

static const BitBoard kBishopMoves[] = {};

static const std::pair<int, int> kKnightDirections[] = {
    {1, 0}, {-1, 0}, {0, -1}, {0, 1}};

}  // namespace

MoveList ChessBoard::GeneratePseudolegalMoves() const {
    MoveList result;
    if (our_king_ == 0) return result;
    for (auto source : our_pieces_) {
        // King
        if (source == our_king_) {
            for (const auto& delta : kKingMoves) {
                const auto dst_row = source.row() + delta.first;
                const auto dst_col = source.col() + delta.second;
                if (!BoardSquare::IsValid(dst_row, dst_col)) continue;
                const BoardSquare destination(dst_row, dst_col);
                if (our_pieces_.get(destination)) continue;
                if (IsUnderAttack(destination)) continue;
                result.emplace_back(source, destination);
            }
            continue;
        }
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
            for (const auto& direction : kBishopDirections) {
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
        if (processed_piece) continue;
        // Pawns.
        if ((pawns_ * kPawnMask).get(source)) {
            // Moves forward.
            {
                const auto dst_row = source.row() + 1;
                const auto dst_col = source.col();
                const BoardSquare destination(dst_row, dst_col);

                if (!our_pieces_.get(destination) &&
                    !their_pieces_.get(destination)) {
                    if (dst_row != 7) {
                        result.emplace_back(source, destination);
                        if (dst_row == 2) {
                            // Maybe it'll be possible to move two squares.
                            if (!our_pieces_.get(3, dst_col) &&
                                !their_pieces_.get(3, dst_col)) {
                                result.emplace_back(source,
                                                    BoardSquare(3, dst_col));
                            }
                        }
                    } else {
                        // Promotions
                        for (auto promotion : kPromotions) {
                            result.emplace_back(source, destination, promotion);
                        }
                    }
                }
            }
            // Captures.
            {
                for (auto direction : {-1, 1}) {
                    const auto dst_row = source.row() + 1;
                    const auto dst_col = source.col() + direction;
                    if (dst_col < 0 || dst_col >= 8) continue;
                    const BoardSquare destination(dst_row, dst_col);
                    if (their_pieces_.get(destination)) {
                        if (dst_row == 7) {
                            // Promotion.
                            for (auto promotion : kPromotions) {
                                result.emplace_back(source, destination,
                                                    promotion);
                            }
                        } else {
                            // Ordinary capture.
                            result.emplace_back(source, destination);
                        }
                    } else if (dst_row == 5 && pawns_.get(7, dst_col)) {
                        // En passant.
                        // "Pawn" on opponent's file 8 means that en passant is
                        // possible. Those fake pawns are reset in ApplyMove.
                        result.emplace_back(source, destination);
                    }
                }
            }
            continue;
        }
        // Knight.
        {
            for (const auto destination : kKnightAttacks[source.as_int()]) {
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
        castlings_.reset_we_can_00();
        castlings_.reset_we_can_000();
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

    // Promotion
    if (move.promotion() != Move::Promotion::None) {
        switch (move.promotion()) {
            case Move::Promotion::Rook:
                rooks_.set(to);
                break;
            case Move::Promotion::Bishop:
                bishops_.set(to);
                break;
            case Move::Promotion::Queen:
                rooks_.set(to);
                bishops_.set(to);
                break;
            default:;
        }
        pawns_.reset(from);
        return true;
    }

    // Reset castling rights.
    if (from.as_int() == 0) {
        castlings_.reset_we_can_000();
    }
    if (from.as_int() == 7) {
        castlings_.reset_we_can_00();
    }

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
        const int krow = their_king_.row();
        const int kcol = their_king_.col();
        if (std::abs(krow - row) <= 1 && std::abs(kcol - col) <= 1) return true;
    }
    // Check Rooks (and queen)
    if (kRookAttacks[square.as_int()].intersects(their_pieces_ * rooks_)) {
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
    }
    // Check Bishops
    if (kBishopAttacks[square.as_int()].intersects(their_pieces_ * bishops_)) {
        for (const auto& direction : kBishopDirections) {
            auto dst_row = row;
            auto dst_col = col;
            while (true) {
                dst_row += direction.first;
                dst_col += direction.second;
                if (!BoardSquare::IsValid(dst_row, dst_col)) break;
                const BoardSquare destination(dst_row, dst_col);
                if (our_pieces_.get(destination)) break;
                if (their_pieces_.get(destination)) {
                    if (bishops_.get(destination)) return true;
                    break;
                }
            }
        }
    }
    // Check pawns
    if (kPawnAttacks[square.as_int()].intersects(their_pieces_ * pawns_)) {
        return true;
    }
    // Check knights
    {
        if (kKnightAttacks[square.as_int()].intersects(
                their_pieces_ - their_king_ - rooks_ - bishops_ -
                (pawns_ * kPawnMask))) {
            return true;
        }
    }
    return false;
}

bool ChessBoard::IsLegalMove(Move move, bool was_under_check) const {
    const auto& from = move.from();
    const auto& to = move.to();

    // If we are already under check, also apply move and check if valid.
    // TODO(mooskagh) Optimize this case
    if (was_under_check) {
        ChessBoard board(*this);
        board.ApplyMove(move);
        return !board.IsUnderCheck();
    }

    // En passant. Complex but rare. Just apply
    // and check that we are not under check.
    if (from.row() == 4 && pawns_.get(from) && from.col() != to.col() &&
        pawns_.get(7, to.col())) {
        ChessBoard board(*this);
        board.ApplyMove(move);
        return !board.IsUnderCheck();
    }

    // If it's kings move, check that destination
    // is not under attack.
    if (from == our_king_) {
        // Castlings were checked earlier.
        if (std::abs(static_cast<int>(from.col()) -
                     static_cast<int>(to.col())) > 1)
            return true;
        return !IsUnderAttack(to);
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
    const bool was_under_check = IsUnderCheck();
    MoveList move_list = GeneratePseudolegalMoves();
    MoveList result;
    result.reserve(move_list.size());

    for (Move m : move_list) {
        if (IsLegalMove(m, was_under_check)) result.emplace_back(m);
    }

    return result;
}

std::vector<MoveExecution> ChessBoard::GenerateLegalMovesAndPositions() const {
    MoveList move_list = GeneratePseudolegalMoves();
    std::vector<MoveExecution> result;

    for (const auto& move : move_list) {
        result.emplace_back();
        auto& newboard = result.back().board;
        newboard = *this;
        result.back().reset_50_moves = newboard.ApplyMove(move);
        if (newboard.IsUnderCheck()) {
            result.pop_back();
            continue;
        }
        result.back().move = move;
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

    if (castlings != "-") {
        for (char c : castlings) {
            switch (c) {
                case 'K':
                    castlings_.set_we_can_00();
                    break;
                case 'k':
                    castlings_.set_they_can_00();
                    break;
                case 'Q':
                    castlings_.set_we_can_000();
                    break;
                case 'q':
                    castlings_.set_they_can_000();
                    break;
                default:
                    throw Exception("Bad fen string: " + fen);
            }
        }
    }

    if (en_passant != "-") {
        auto square = BoardSquare(en_passant);
        if (square.row() != 2 && square.row() != 5)
            throw Exception("Bad fen string: " + fen +
                            " wrong en passant rank");
        pawns_.set((square.row() == 2) ? 0 : 7, square.col());
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
