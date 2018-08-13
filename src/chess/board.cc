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
static const std::pair<int, int> kPawnMoves[] = {
	{1, 0}, {0,  1}, {0, -1}}
  
static const std::pair<int, int> kKingMoves[] = {
	{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

static const std::pair<int, int> kBishopMoves[] = {
	{2, 2}, {2, -2}, {-2, 2}, {-2, -2}};

static const std::pair<int, int> kKnightMoves[] = {
	{1, 2}, {-1, 2}, {1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}, {-2, -1}};
  
static const std::pair<int, int> kAdvisorMoves[] = {
	{1, 1}, {-1, 1}, {1, -1}, {-1, -1}}; 
  
// include kCannonDirection
static const std::pair<int, int> kRookDirections[] = {
	{1, 0}, {-1, 0}, {0, 1}, {0, -1}}; 
  
// Tables for attack.
// Each square goes from left to right, from bottom to top:
  
// Which squares can advisor attack from its squares.
static const BitBoard kAdvisorAttacks[] = {
    0x00000000000000000002000, 0x00000000000000000002000, 0x00000000000000000A00028,
    0x00000000000000000002000, 0x00000000000000000002000};
  
static const std::unordered_map<std::uint8_t, BitBoard> kAdvisorPos = {
    {3,  kAdvisorAttacks[0]}, {5,  kAdvisorAttacks[1]}, {13, kAdvisorAttacks[2]},
    {21, kAdvisorAttacks[3]}, {23, kAdvisorAttacks[4]}};
  
// Which squares can bishop attack from its squares.
static const BitBoard kBishopAttacks[] = {
    0x00000000000000004400000, 0x00000000000000000440000, 0x00000000000040000000040,
    0x00000000000044000000044, 0x00000000000004000000004, 0x00000000000000004400000,
    0x00000000000000000440000};
 
static const std::unordered_map<std::uint8_t, BitBoard> kBishopPos = {
    {2,  kBishopAttacks[0]}, {6,  kBishopAttacks[1]}, {18, kBishopAttacks[2]},
    {22, kBishopAttacks[3]}, {26, kBishopAttacks[4]}, {38, kBishopAttacks[5]},
    {42, kBishopAttacks[6]}};
 
// Which squares can king attack from its squares.
static const BitBoard kKingAttacks[] = {
    0x00000000000000000004010, 0x00000000000000000002028, 0x00000000000000000001010,
    0x00000000000000000802020, 0x00000000000000000405010, 0x00000000000000000202008,
    0x00000000000000000404000, 0x00000000000000000A02000, 0x00000000000000000401000};
  
static const std::unordered_map<std::uint8_t, BitBoard> kKingPos = {
    {3,  kKingAttacks[0]}, {4,  kKingAttacks[1]}, {5,  kKingAttacks[2]},
    {12, kKingAttacks[3]}, {13, kKingAttacks[4]}, {13, kKingAttacks[5]}
    {21, kKingAttacks[6]}, {22, kKingAttacks[7]}, {23, kKingAttacks[8]}};
  
// Which squares can pawn attack from every of squares.
static const BitBoard kPawnAttacks[] = {
    0x00000000000000000000000, 0x00000000000000000000000, 0x00000000000000000000000, 
    0x00000000000000000000000, 0x00000000000000000000000, 0x00000000000000000000000, 
    0x00000000000000000000000, 0x00000000000000000000000, 0x00000000000000000000000, 
    0x00000000000000000000000, 0x00000000000000000000000, 0x00000000000000000000000, 
    0x00000000000000000000000, 0x00000000000000000000000, 0x00000000000000000000000, 
    0x00000000000000000000000, 0x00000000000000000000000, 0x00000000000000000000000, 
    0x00000000000000000000000, 0x00000000000000000000000, 0x00000000000000000000000, 
    0x00000000000000000000000, 0x00000000000000000000000, 0x00000000000000000000000, 
    0x00000000000000000000000, 0x00000000000000000000000, 0x00000000000000000000000, 
    0x00000000000100000000000, 0x00000000000000000000000, 0x00000000000040000000000, 
    0x00000000000000000000000, 0x00000000000010000000000, 0x00000000000000000000000, 
    0x00000000000004000000000, 0x00000000000000000000000, 0x00000000000001000000000, 
    0x00000000020000000000000, 0x00000000000000000000000, 0x00000000008000000000000, 
    0x00000000000000000000000, 0x00000000002000000000000, 0x00000000000000000000000,
    0x00000000000800000000000, 0x00000000000000000000000, 0x00000000000200000000000, 
    0x00000004010000000000000, 0x00000002028000000000000, 0x00000001014000000000000, 
    0x0000000080A000000000000, 0x00000000405000000000000, 0x00000000202800000000000, 
    0x00000000101400000000000, 0x00000000080A00000000000, 0x00000000040400000000000, 
    0x00000802000000000000000, 0x00000405000000000000000, 0x00000202800000000000000, 
    0x00000101400000000000000, 0x00000080A00000000000000, 0x00000040500000000000000, 
    0x00000020280000000000000, 0x00000010140000000000000, 0x00000008080000000000000, 
    0x00100400000000000000000, 0x00080A00000000000000000, 0x00040500000000000000000,
    0x00020280000000000000000, 0x00010140000000000000000, 0x000080A0000000000000000, 
    0x00004050000000000000000, 0x00002028000000000000000, 0x00001010000000000000000, 
    0x20080000000000000000000, 0x10140000000000000000000, 0x080A0000000000000000000, 
    0x04050000000000000000000, 0x02028000000000000000000, 0x01014000000000000000000,
    0x0080A000000000000000000, 0x00405000000000000000000, 0x00202000000000000000000,
    0x10000000000000000000000, 0x28000000000000000000000, 0x14000000000000000000000, 
    0x0A000000000000000000000, 0x05000000000000000000000, 0x02800000000000000000000, 
    0x01400000000000000000000, 0x00A00000000000000000000, 0x00400000000000000000000};
  
// Which squares can rook attack from every of squares.
static const BitBoard kRookAttacks[] = {
    0x201008040201008040200FF, 0x1008040201008040201017F, 0x080402010080402010081BF,
    0x040201008040201008041DF, 0x020100804020100804021EF, 0x010080402010080402011F7,
    0x008040201008040201009FB, 0x004020100804020100805FD, 0x002010080402010080403FE,
    0x2010080402010080401FF00, 0x1008040201008040202FE80, 0x08040201008040201037E40,
    0x0402010080402010083BE20, 0x0201008040201008043DE10, 0x0100804020100804023EE08,
    0x0080402010080402013F604, 0x004020100804020100BFA02, 0x0020100804020100807FC01,
    0x20100804020100803FE0100, 0x10080402010080405FD0080, 0x08040201008040206FC8040,
    0x040201008040201077C4020, 0x02010080402010087BC2010, 0x01008040201008047DC1008,
    0x00804020100804027EC0804, 0x00402010080402017F40402, 0x0020100804020100FF80201,
    0x201008040201007FC020100, 0x10080402010080BFA010080, 0x08040201008040DF9008040,
    0x04020100804020EF8804020, 0x02010080402010F78402010, 0x01008040201008FB8201008,
    0x00804020100804FD8100804, 0x00402010080402FE8080402, 0x00201008040201FF0040201,
    0x201008040200FF804020100, 0x1008040201017F402010080, 0x080402010081BF201008040,
    0x040201008041DF100804020, 0x020100804021EF080402010, 0x010080402011F7040201008,
    0x008040201009FB020100804, 0x004020100805FD010080402, 0x002010080403FE008040201,
    0x2010080401FF00804020100, 0x1008040202FE80402010080, 0x08040201037E40201008040,
    0x0402010083BE20100804020, 0x0201008043DE10080402010, 0x0100804023EE08040201008,
    0x0080402013F604020100804, 0x004020100BFA02010080402, 0x0020100807FC01008040201,
    0x20100803FE0100804020100, 0x10080405FD0080402010080, 0x08040206FC8040201008040,
    0x040201077C4020100804020, 0x02010087BC2010080402010, 0x01008047DC1008040201008,
    0x00804027EC0804020100804, 0x00402017F40402010080402, 0x0020100FF80201008040201,
    0x201007FC020100804020100, 0x10080BFA010080402010080, 0x08040DF9008040201008040,
    0x04020EF8804020100804020, 0x02010F78402010080402010, 0x01008FB8201008040201008,
    0x00804FD8100804020100804, 0x00402FE8080402010080402, 0x00201FF0040201008040201,
    0x200FF804020100804020100, 0x1017F402010080402010080, 0x081BF201008040201008040,
    0x041DF100804020100804020, 0x021EF080402010080402010, 0x011F7040201008040201008,
    0x009FB020100804020100804, 0x005FD010080402010080402, 0x003FE008040201008040201,
    0x1FF00804020100804020100, 0x2FE80402010080402010080, 0x37E40201008040201008040,
    0x3BE20100804020100804020, 0x3DE10080402010080402010, 0x3EE08040201008040201008,
    0x3F604020100804020100804, 0x3FA02010080402010080402, 0x3FC01008040201008040201};
  
// Which squares can knight attack from every of squares.
static const BitBoard kKnightAttacks[] = {
    0x00000000000000002008000, 0x00000000000000005004000, 0x00000000000000002822000,
    0x00000000000000001411000, 0x00000000000000000A08800, 0x00000000000000000504400,
    0x00000000000000000282200, 0x00000000000000000141000, 0x00000000000000000080800,
    0x00000000000000401000040, 0x00000000000000A00800020, 0x00000000000000504400110,
    0x00000000000000282200088, 0x00000000000000141100044, 0x000000000000000A0880022,
    0x00000000000000050440011, 0x00000000000000028200008, 0x00000000000000010100004,
    0x00000000000080200008080, 0x00000000000140100004140, 0x000000000000A08800220A0,
    0x00000000000050440011050, 0x00000000000028220008828, 0x00000000000014110004414,
    0x0000000000000A08800220A, 0x00000000000005040001005, 0x00000000000002020000802,
    0x00000000010040001010000, 0x00000000028020000828000, 0x00000000014110004414000,
    0x0000000000A08800220A000, 0x00000000005044001105000, 0x00000000002822000882800,
    0x00000000001411000441400, 0x00000000000A08000200A00, 0x00000000000404000100400,
    0x00000002008000202000000, 0x00000005004000105000000, 0x00000002822000882800000, 
    0x00000001411000441400000, 0x00000000A08800220A00000, 0x00000000504400110500000, 
    0x00000000282200088280000, 0x00000000141000040140000, 0x00000000080800020080000, 
    0x00000401000040400000000, 0x00000A00800020A00000000, 0x00000504400110500000000, 
    0x00000282200088280000000, 0x00000141100044140000000, 0x000000A08800220A0000000, 
    0x00000050440011050000000, 0x00000028200008028000000, 0x00000010100004010000000, 
    0x00080200008080000000000, 0x00140100004140000000000, 0x000A08800220A0000000000, 
    0x00050440011050000000000, 0x00028220008828000000000, 0x00014110004414000000000, 
    0x0000A08800220A000000000, 0x00005040001005000000000, 0x00002020000802000000000, 
    0x10040001010000000000000, 0x28020000828000000000000, 0x14110004414000000000000,
    0x0A08800220A000000000000, 0x05044001105000000000000, 0x02822000882800000000000,
    0x01411000441400000000000, 0x00A08000200A00000000000, 0x00404000100400000000000,
    0x08000202000000000000000, 0x04000105000000000000000, 0x22000882800000000000000,
    0x11000441400000000000000, 0x08800220A00000000000000, 0x04400110500000000000000,
    0x02200088280000000000000, 0x01000040140000000000000, 0x00800020080000000000000,
    0x00040400000000000000000, 0x00020A00000000000000000, 0x00110500000000000000000,
    0x00088280000000000000000, 0x00044140000000000000000, 0x000220A0000000000000000,
    0x00011050000000000000000, 0x00008028000000000000000, 0x00004010000000000000000};
}  // namespace

MoveList ChessBoard::GeneratePseudolegalMoves() const {
    MoveList result;
    // King
    for (const auto destination : kKingPos.find(our_king_.as_int()).second) {
        if (our_pieces_.get(destination)) continue;
        if (IsUnderAttack(destination)) continue;
        result.emplace_back(our_king_, destination);
    }
    // Other pieces
    for (auto source : our_pieces_ - our_king_) {
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
            continue;
        }
        // Bishop
        if (bishops_.get(source)) {
            for (const auto destination : kBishopPos.find(source.as_int()).second) {
                if (our_pieces_.get(destination)) continue;
                const BoardSquare block((source.row()+destination.row())/2, (source.col()+destination.col())/2);
                if (our_pieces_.get(block) || their_pieces_.get(block)) continue;
                result.emplace_back(source, destination);
            }
            continue;
        }
        // Pawns
        if (pawns_.get(source)) {
            for (const auto destination : kPawnAttacks[source.as_int()]) {
                if (our_pieces_.get(destination)) continue;
                result.emplace_back(source, destination);
            }
            continue;
        }
        // Knight
        if (knights_.get(source)) {
            for (const auto destination : kKnightAttacks[source.as_int()]) {
                if (our_pieces_.get(destination)) continue;
                const BoardSquare block((source.row()+destination.row())/2, (source.col()+destination.col())/2);
                if (our_pieces_.get(block) || their_pieces_.get(block)) continue;
                result.emplace_back(source, destination);
            }
            continue;
        }
        // Cannon
        if (cannons_.get(source)){
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
            continue;
        }
        // Advisor
        if (advisors_.get(source)) {
            for (const auto destination : kAdvisorPos.find(source.as_int()).second) {
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
    bool reset_60_moves = their_pieces_.get(to);
    their_pieces_.reset(to);
    rooks_.reset(to);
    knights_.reset(to);
    cannons_.reset(to);
    bishops_.reset(to);
    advisors_.reset(to);
    pawns_.reset(to);
        
    return reset_60_moves;

    // Now destination square for our piece is known.
    our_pieces_.set(to);

    // Ordinary move.
    rooks_.set_if(to, rooks_.get(from));
    knights_.set_if(to, knights_.get(from));
    cannons_.set_if(to, cannons_.get(from));
    bishops_.set_if(to, bishops_.get(from));
    advisors_.set_if(to, advisors_.get(from));
    pawns_.set_if(to, pawns_.get(from));
    rooks_.reset(from);
    knights_.reset(from);
    cannons_.reset(from);
    bishops_.reset(from);
    advisors_.reset(from);
    pawns_.reset(from);

    return reset_60_moves;
}

bool ChessBoard::IsUnderAttack(BoardSquare square) const {
    const int row = square.row();
    const int col = square.col();
    // Check king
    if (our_king_.get(square)) {
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
            if (face == true) return true;
        }
    }
    else {
        for (const auto& delta : kKingMoves) {
            auto dst_row = row + delta.first;
            auto dst_col = col + delta.second; 
            const BoardSquare destination(dst_row, dst_col);
            if (our_pieces_.get(destination)) break;
            if (their_pieces_get(destination)) {
                if (their_king_.get(destination)) return true;
            break;
            }
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
    for (const auto& delta : kPawnMoves) {
        auto dst_row = row + delta.first;
        auto dst_col = col + delta.second; 
        const BoardSquare destination(dst_row, dst_col);
        if (our_pieces_.get(destination)) break;
        if (their_pieces_get(destination)) {
            if (pawns_.get(destination)) return true;
            break;
        }
    }
    // Check knights
    for (const auto& delta : kKnightMoves) {
        auto dst_row = row + delta.first;
        auto dst_col = col + delta.second;
        if (!BoardSquare::IsValid(dst_row, dst_col)) continue;
        const BoardSquare destination(dst_row, dst_col);
        if (knights_.get(destination) && their_pieces_.get(destination)) {
            const BoardSquare block(dst_row+delta.first/2,dst_col+delta.second/2);
            if (!our_pieses_.get(block) && !their_pieces_.get(block)) return true
        }
    }
    // Check Cannons
    for (const auto& direction : kRookDirections) {
        auto dst_row = row;
        auto dst_col = col;
        int count_block = 0;
        while (true) {
            dst_row += direction.first;
            dst_col += direction.second;
            if (!BoardSquare::IsValid(dst_row, dst_col)) break;
            const BoardSquare destination(dst_row, dst_col);
            if (our_pieces_.get(destination) || their_pieces_.get(destination)) count_block++;
            if (count_block == 1) continue;
            if (count_block == 2) {
                if(Cannons_.get(destination) && their_pieces_.get(destination)) return true;
            }
            if (count_block == 2) break;
    }
    return false;
}

bool ChessBoard::IsLegalMove(Move move) const {
    const auto& from = move.from();
    const auto& to = move.to();
    
    // If it's kings move, check that destination
    // is not under attack.
    if (from == our_king_) return true;

    ChessBoard board(*this);
    board.ApplyMove(move);
    if (board.IsUnderAttack(board.our_king_)) return false;
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
    int row = 8;
    int col = 0;

    std::istringstream fen_str(fen);
    string board;
    string who_to_move;
    int no_capture_halfmoves;
    int total_moves;
    fen_str >> board >> who_to_move >> no_capture_halfmoves >> total_moves;

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
        } else if (c == 'P' || c == 'p') {
            pawns_.set(row, col);
        } else if (c == 'N' || c == 'n') {
            knights_.set(row, col);
        } else if (c == 'A' || c == 'a') {
            advisors_.set(row, col);
        } else if (c == 'C' || c == 'c') {
            cannons_.set(row, col);
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
