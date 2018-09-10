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
    (0x000000000000ULL, 0x000000002000ULL), (0x000000000000ULL, 0x000000002000ULL),
    (0x000000000000ULL, 0x000000A00028ULL), (0x000000000000ULL, 0x000000002000ULL),
    (0x000000000000ULL, 0x000000002000ULL)};
  
static const std::unordered_map<std::uint8_t, BitBoard> kAdvisorPos = {
    {3,  kAdvisorAttacks[0]}, {5,  kAdvisorAttacks[1]}, {13, kAdvisorAttacks[2]},
    {21, kAdvisorAttacks[3]}, {23, kAdvisorAttacks[4]}};
  
// Which squares can bishop attack from its squares.
static const BitBoard kBishopAttacks[] = {
    (0x000000000000ULL, 0x000004400000ULL), (0x000000000000ULL, 0x000000440000ULL),
    (0x000000000000ULL, 0x040000000040ULL), (0x000000000000ULL, 0x044000000044ULL),
    (0x000000000000ULL, 0x004000000004ULL), (0x000000000000ULL, 0x000004400000ULL),
    (0x000000000000ULL, 0x000000440000ULL)};
 
static const std::unordered_map<std::uint8_t, BitBoard> kBishopPos = {
    {2,  kBishopAttacks[0]}, {6,  kBishopAttacks[1]}, {18, kBishopAttacks[2]},
    {22, kBishopAttacks[3]}, {26, kBishopAttacks[4]}, {38, kBishopAttacks[5]},
    {42, kBishopAttacks[6]}};
 
// Which squares can king attack from its squares.
static const BitBoard kKingAttacks[] = {
    (0x000000000000ULL, 0x000000004010ULL), (0x000000000000ULL, 0x000000002028ULL),
    (0x000000000000ULL, 0x000000001010ULL), (0x000000000000ULL, 0x000000802020ULL),
    (0x000000000000ULL, 0x000000405010ULL), (0x000000000000ULL, 0x000000202008ULL),
    (0x000000000000ULL, 0x000000404000ULL), (0x000000000000ULL, 0x000000A02000ULL),
    (0x000000000000ULL, 0x000000401000ULL)};
  
static const std::unordered_map<std::uint8_t, BitBoard> kKingPos = {
    {3,  kKingAttacks[0]}, {4,  kKingAttacks[1]}, {5,  kKingAttacks[2]},
    {12, kKingAttacks[3]}, {13, kKingAttacks[4]}, {13, kKingAttacks[5]}
    {21, kKingAttacks[6]}, {22, kKingAttacks[7]}, {23, kKingAttacks[8]}};
  
// Which squares can pawn attack from every of squares.
static const BitBoard kPawnAttacks[] = {
    (0x000000000000ULL, 0x100000000000ULL), (0x000000000000ULL, 0x040000000000ULL),
    (0x000000000000ULL, 0x010000000000ULL), (0x000000000000ULL, 0x004000000000ULL),
    (0x000000000000ULL, 0x001000000000ULL), (0x000000000100ULL, 0x000000000000ULL),
    (0x000000000040ULL, 0x000000000000ULL), (0x000000000010ULL, 0x000000000000ULL),
    (0x000000000004ULL, 0x000000000000ULL), (0x000000000001ULL, 0x000000000000ULL),
    (0x000000020080ULL, 0x000000000000ULL), (0x000000010140ULL, 0x000000000000ULL),
    (0x0000000080A0ULL, 0x000000000000ULL), (0x000000004050ULL, 0x000000000000ULL),
    (0x000000002028ULL, 0x000000000000ULL), (0x000000001014ULL, 0x000000000000ULL),
    (0x00000000080AULL, 0x000000000000ULL), (0x000000000405ULL, 0x000000000000ULL),
    (0x000000000202ULL, 0x000000000000ULL), (0x000004010000ULL, 0x000000000000ULL),
    (0x000002028000ULL, 0x000000000000ULL), (0x000001014000ULL, 0x000000000000ULL),
    (0x00000080A000ULL, 0x000000000000ULL), (0x000000405000ULL, 0x000000000000ULL),
    (0x000000202800ULL, 0x000000000000ULL), (0x000000101400ULL, 0x000000000000ULL),
    (0x000000080A00ULL, 0x000000000000ULL), (0x000000040400ULL, 0x000000000000ULL),
    (0x000802000000ULL, 0x000000000000ULL), (0x000405000000ULL, 0x000000000000ULL),
    (0x000202800000ULL, 0x000000000000ULL), (0x000101400000ULL, 0x000000000000ULL),
    (0x000080A00000ULL, 0x000000000000ULL), (0x000040500000ULL, 0x000000000000ULL),
    (0x000020280000ULL, 0x000000000000ULL), (0x000010140000ULL, 0x000000000000ULL),
    (0x000008080000ULL, 0x000000000000ULL), (0x100400000000ULL, 0x000000000000ULL),
    (0x080A00000000ULL, 0x000000000000ULL), (0x040500000000ULL, 0x000000000000ULL),
    (0x020280000000ULL, 0x000000000000ULL), (0x010140000000ULL, 0x000000000000ULL),
    (0x0080A0000000ULL, 0x000000000000ULL), (0x004050000000ULL, 0x000000000000ULL),
    (0x002028000000ULL, 0x000000000000ULL), (0x001010000000ULL, 0x000000000000ULL),
    (0x080000000000ULL, 0x000000000000ULL), (0x140000000000ULL, 0x000000000000ULL),
    (0x0A0000000000ULL, 0x000000000000ULL), (0x050000000000ULL, 0x000000000000ULL),
    (0x028000000000ULL, 0x000000000000ULL), (0x014000000000ULL, 0x000000000000ULL),
    (0x00A000000000ULL, 0x000000000000ULL), (0x005000000000ULL, 0x000000000000ULL),
    (0x002000000000ULL, 0x000000000000ULL)};
  
// Which squares can rook attack from every of squares.
static const BitBoard kRookAttacks[] = {
    (0x100804020100ULL, 0x1008040200FFULL), (0x080402010080ULL, 0x08040201017FULL),
    (0x040201008040ULL, 0x0402010081BFULL), (0x020100804020ULL, 0x0201008041DFULL),
    (0x010080402010ULL, 0x0100804021EFULL), (0x008040201008ULL, 0x0080402011F7ULL),
    (0x004020100804ULL, 0x0040201009FBULL), (0x002010080402ULL, 0x0020100805FDULL),
    (0x001008040201ULL, 0x0010080403FEULL), (0x100804020100ULL, 0x10080401FF00ULL),
    (0x080402010080ULL, 0x08040202FE80ULL), (0x040201008040ULL, 0x040201037E40ULL),
    (0x020100804020ULL, 0x02010083BE20ULL), (0x010080402010ULL, 0x01008043DE10ULL),
    (0x008040201008ULL, 0x00804023EE08ULL), (0x004020100804ULL, 0x00402013F604ULL),
    (0x002010080402ULL, 0x0020100BFA02ULL), (0x001008040201ULL, 0x00100807FC01ULL),
    (0x100804020100ULL, 0x100803FE0100ULL), (0x080402010080ULL, 0x080405FD0080ULL),
    (0x040201008040ULL, 0x040206FC8040ULL), (0x020100804020ULL, 0x0201077C4020ULL),
    (0x010080402010ULL, 0x010087BC2010ULL), (0x008040201008ULL, 0x008047DC1008ULL),
    (0x004020100804ULL, 0x004027EC0804ULL), (0x002010080402ULL, 0x002017F40402ULL),
    (0x001008040201ULL, 0x00100FF80201ULL), (0x100804020100ULL, 0x1007FC020100ULL),
    (0x080402010080ULL, 0x080BFA010080ULL), (0x040201008040ULL, 0x040DF9008040ULL),
    (0x020100804020ULL, 0x020EF8804020ULL), (0x010080402010ULL, 0x010F78402010ULL),
    (0x008040201008ULL, 0x008FB8201008ULL), (0x004020100804ULL, 0x004FD8100804ULL),
    (0x002010080402ULL, 0x002FE8080402ULL), (0x001008040201ULL, 0x001FF0040201ULL),
    (0x100804020100ULL, 0x0FF804020100ULL), (0x080402010080ULL, 0x17F402010080ULL),
    (0x040201008040ULL, 0x1BF201008040ULL), (0x020100804020ULL, 0x1DF100804020ULL),
    (0x010080402010ULL, 0x1EF080402010ULL), (0x008040201008ULL, 0x1F7040201008ULL),
    (0x004020100804ULL, 0x1FB020100804ULL), (0x002010080402ULL, 0x1FD010080402ULL),
    (0x001008040201ULL, 0x1FE008040201ULL), (0x1008040200FFULL, 0x100804020100ULL),
    (0x08040201017FULL, 0x080402010080ULL), (0x0402010081BFULL, 0x040201008040ULL),
    (0x0201008041DFULL, 0x020100804020ULL), (0x0100804021EFULL, 0x010080402010ULL),
    (0x0080402011F7ULL, 0x008040201008ULL), (0x0040201009FBULL, 0x004020100804ULL),
    (0x0020100805FDULL, 0x002010080402ULL), (0x0010080403FEULL, 0x001008040201ULL),
    (0x10080401FF00ULL, 0x100804020100ULL), (0x08040202FE80ULL, 0x080402010080ULL),
    (0x040201037E40ULL, 0x040201008040ULL), (0x02010083BE20ULL, 0x020100804020ULL),
    (0x01008043DE10ULL, 0x010080402010ULL), (0x00804023EE08ULL, 0x008040201008ULL),
    (0x00402013F604ULL, 0x004020100804ULL), (0x0020100BFA02ULL, 0x002010080402ULL),
    (0x00100807FC01ULL, 0x001008040201ULL), (0x100803FE0100ULL, 0x100804020100ULL),
    (0x080405FD0080ULL, 0x080402010080ULL), (0x040206FC8040ULL, 0x040201008040ULL),
    (0x0201077C4020ULL, 0x020100804020ULL), (0x010087BC2010ULL, 0x010080402010ULL),
    (0x008047DC1008ULL, 0x008040201008ULL), (0x004027EC0804ULL, 0x004020100804ULL),
    (0x002017F40402ULL, 0x002010080402ULL), (0x00100FF80201ULL, 0x001008040201ULL),
    (0x1007FC020100ULL, 0x100804020100ULL), (0x080BFA010080ULL, 0x080402010080ULL),
    (0x040DF9008040ULL, 0x040201008040ULL), (0x020EF8804020ULL, 0x020100804020ULL),
    (0x010F78402010ULL, 0x010080402010ULL), (0x008FB8201008ULL, 0x008040201008ULL),
    (0x004FD8100804ULL, 0x004020100804ULL), (0x002FE8080402ULL, 0x002010080402ULL),
    (0x001FF0040201ULL, 0x001008040201ULL), (0x0FF804020100ULL, 0x100804020100ULL),
    (0x17F402010080ULL, 0x080402010080ULL), (0x1BF201008040ULL, 0x040201008040ULL),
    (0x1DF100804020ULL, 0x020100804020ULL), (0x1EF080402010ULL, 0x010080402010ULL),
    (0x1F7040201008ULL, 0x008040201008ULL), (0x1FB020100804ULL, 0x004020100804ULL),
    (0x1FD010080402ULL, 0x002010080402ULL), (0x1FE008040201ULL, 0x001008040201ULL)};
  
// Which squares can knight attack from every of squares.
static const BitBoard kKnightAttacks[] = {
    (0x000000000000ULL, 0x000002008000ULL), (0x000000000000ULL, 0x000005004000ULL),
    (0x000000000000ULL, 0x000002822000ULL), (0x000000000000ULL, 0x000001411000ULL),
    (0x000000000000ULL, 0x000000A08800ULL), (0x000000000000ULL, 0x000000504400ULL),
    (0x000000000000ULL, 0x000000282200ULL), (0x000000000000ULL, 0x000000141000ULL),
    (0x000000000000ULL, 0x000000080800ULL), (0x000000000000ULL, 0x000401000040ULL),
    (0x000000000000ULL, 0x000A00800020ULL), (0x000000000000ULL, 0x000504400110ULL),
    (0x000000000000ULL, 0x000282200088ULL), (0x000000000000ULL, 0x000141100044ULL),
    (0x000000000000ULL, 0x0000A0880022ULL), (0x000000000000ULL, 0x000050440011ULL),
    (0x000000000000ULL, 0x000028200008ULL), (0x000000000000ULL, 0x000010100004ULL),
    (0x000000000000ULL, 0x080200008080ULL), (0x000000000000ULL, 0x140100004140ULL),
    (0x000000000000ULL, 0x0A08800220A0ULL), (0x000000000000ULL, 0x050440011050ULL),
    (0x000000000000ULL, 0x028220008828ULL), (0x000000000000ULL, 0x014110004414ULL),
    (0x000000000000ULL, 0x00A08800220AULL), (0x000000000000ULL, 0x005040001005ULL),
    (0x000000000000ULL, 0x002020000802ULL), (0x000000000080ULL, 0x040001010000ULL),
    (0x000000000140ULL, 0x020000828000ULL), (0x0000000000A0ULL, 0x110004414000ULL),
    (0x000000000050ULL, 0x08800220A000ULL), (0x000000000028ULL, 0x044001105000ULL),
    (0x000000000014ULL, 0x022000882800ULL), (0x00000000000AULL, 0x011000441400ULL),
    (0x000000000005ULL, 0x008000200A00ULL), (0x000000000002ULL, 0x004000100400ULL),
    (0x000000010040ULL, 0x000202000000ULL), (0x000000028020ULL, 0x000105000000ULL),
    (0x000000014110ULL, 0x000882800000ULL), (0x00000000A088ULL, 0x000441400000ULL),
    (0x000000005044ULL, 0x000220A00000ULL), (0x000000002822ULL, 0x000110500000ULL),
    (0x000000001411ULL, 0x000088280000ULL), (0x000000000A08ULL, 0x000040140000ULL),
    (0x000000000404ULL, 0x000020080000ULL), (0x000002008000ULL, 0x040400000000ULL),
    (0x000005004000ULL, 0x020A00000000ULL), (0x000002822000ULL, 0x110500000000ULL),
    (0x000001411000ULL, 0x088280000000ULL), (0x000000A08800ULL, 0x044140000000ULL),
    (0x000000504400ULL, 0x0220A0000000ULL), (0x000000282200ULL, 0x011050000000ULL),
    (0x000000141000ULL, 0x008028000000ULL), (0x000000080800ULL, 0x004010000000ULL),
    (0x000401000040ULL, 0x080000000000ULL), (0x000A00800020ULL, 0x140000000000ULL),
    (0x000504400110ULL, 0x0A0000000000ULL), (0x000282200088ULL, 0x050000000000ULL),
    (0x000141100044ULL, 0x028000000000ULL), (0x0000A0880022ULL, 0x014000000000ULL),
    (0x000050440011ULL, 0x00A000000000ULL), (0x000028200008ULL, 0x005000000000ULL),
    (0x000010100004ULL, 0x002000000000ULL), (0x080200008080ULL, 0x000000000000ULL),
    (0x140100004140ULL, 0x000000000000ULL), (0x0A08800220A0ULL, 0x000000000000ULL),
    (0x050440011050ULL, 0x000000000000ULL), (0x028220008828ULL, 0x000000000000ULL),
    (0x014110004414ULL, 0x000000000000ULL), (0x00A08800220AULL, 0x000000000000ULL),
    (0x005040001005ULL, 0x000000000000ULL), (0x002020000802ULL, 0x000000000000ULL),
    (0x040001010000ULL, 0x000000000000ULL), (0x020000828000ULL, 0x000000000000ULL),
    (0x110004414000ULL, 0x000000000000ULL), (0x08800220A000ULL, 0x000000000000ULL),
    (0x044001105000ULL, 0x000000000000ULL), (0x022000882800ULL, 0x000000000000ULL),
    (0x011000441400ULL, 0x000000000000ULL), (0x008000200A00ULL, 0x000000000000ULL),
    (0x004000100400ULL, 0x000000000000ULL), (0x000202000000ULL, 0x000000000000ULL),
    (0x000105000000ULL, 0x000000000000ULL), (0x000882800000ULL, 0x000000000000ULL),
    (0x000441400000ULL, 0x000000000000ULL), (0x000220A00000ULL, 0x000000000000ULL),
    (0x000110500000ULL, 0x000000000000ULL), (0x000088280000ULL, 0x000000000000ULL),
    (0x000040140000ULL, 0x000000000000ULL), (0x000020080000ULL, 0x000000000000ULL)};
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
    // Check their pieces that can attack this square:
    // Check king
    if (our_king_.get(square)) {
	      const int krow = their_king_.row();
        const int kcol = their_king_.col();
        if (col == kcol) {
            bool face = true;
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
