#include <stdio.h>

class BitBoard {
   public:
    constexpr BitBoard(__uint128_t board) : board_(board) {}
    constexpr Bitboard(const uint16_t high,const uint16_t low) : value_u64[1](high), value_u64[0](low) {}
    BitBoard(const BitBoard&) = default;
    BitBoard() = default;

    __uint128_t as_int() const { return board_; }
    void clear() { board_ = 0; }

    Bitboard operator&(const Bitboard& other) const {
        return Bitboard(value_u64[1] & other.value_u64[1], value_u64[0] & other.value_u64[0])
    }
    Bitboard operator|(const Bitboard& other) const {
        return Bitboard(value_u64[1] | other.value_u64[1], value_u64[0] | other.value_u64[0])
    }
    Bitboard operator^(const Bitboard& other) const {
        return Bitboard(value_u64[1] ^ other.value_u64[1], value_u64[0] ^ other.value_u64[0]);
    }
    Bitboard operator~(const Bitboard& other) const {
        return Bitboard(value_u64[1] ~ other.value_u64[1], value_u64[0] ~ other.value_u64[0])
    }
    Bitboard operator&=(const Bitboard& other) const {
        this = this & other;
        return *this;
    }
    Bitboard operator|=(const Bitboard& other) const {
        this = this | other;
        return *this;
    }
    Bitboard operator^=(const Bitboard& other) const {
        this = this ^ other;
        return *this;
    }
    Bitboard operator~=(const Bitboard& other) const {
        this = this ~ other;
        return *this;
    }
    Bitboard operator<<(const int n) const {
        return Bitboard((n < 64) ? ((value_u64[1] << n) + (value_u64[0] >> (64 - n)), value_u64[0] << n)
                        : (value_u64[1] << (n-64) , 0)));
    }
    Bitboard operator>>(const int n) const {
        return Bitboard((n < 64) ? (value_u64[1] >> n,(value_u64[1] << (64 - n)) + (value_u64[0] >> n))
                        : (0, value_u64[0] >> (n-64)));
    }
    bool operator==(const BitBoard& other) const {
        return value_u64[1] == other.value_u64[1] && value_u64[0] == other.value_u64[0];
    }

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
    uint16_t value_u64[2] = (0, 0);
};

main(int argc, char const *argv[])
{
    Bitboard A(0xc,0x60000000000);
    A << 4;
    A
    return 0;
}

