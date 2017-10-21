#include "big_integer.h"
#include <algorithm>
#include <climits>
#include <vector>
#include <functional>
#include <cassert>
#include <iostream>

static const uint32_t BASE = UINT32_MAX; //not really base but actually BASE - 1

static uint32_t filler(uint32_t x) { //the thing that we are using if we're filling the number in two complement form
    return (x >> 31 ? BASE : 0);
}

static int maxbit(uint32_t n) { //Pre: n != 0
    int temp = -1;
    while (n) {
        n >>= 1;
        ++temp;
    }
    return temp;
}

big_integer::big_integer() :
    big_integer(0) {
}

big_integer::big_integer(const big_integer &b) :
    size(b.size),
    data(new uint32_t[b.size])
{
    std::copy(b.data, b.data + size, data);
}

big_integer::big_integer(const std::string &s) :
    big_integer(0)
{
    bool neg = s[0] == '-';
    for (size_t i = neg; i < s.size(); ++i) {
        *this *= 10;
        *this += s[i] - '0';
    }
    if (neg)
        *this = -*this;
}

big_integer::big_integer(int b) :
    size(1),
    data(new uint32_t[1])
{
    data[0] = b;
}

big_integer::big_integer(uint32_t b) {
    if (b < (uint32_t)1 << 31) {
        size = 1;
        data = new uint32_t[1];
        data[0] = b;
    }
    else {
        size = 2;
        data = new uint32_t[2];
        data[0] = b;
        data[1] = 0;
    }
}

big_integer::~big_integer() {
    delete[] data;
}

big_integer &big_integer::operator = (const big_integer &b) {
    uint32_t *ndata = new uint32_t[b.size];
    std::copy(b.data, b.data + b.size, ndata);
    delete[] data;
    size = b.size;
    data = ndata;
    return *this;
}

big_integer &big_integer::operator += (const big_integer &b) {
    return *this = *this + b;
}
big_integer &big_integer::operator -= (const big_integer &b) {
    return *this = *this - b;
}
big_integer &big_integer::operator *= (const big_integer &b) {
    return *this = *this * b;
}

big_integer &big_integer::operator /= (const big_integer &b) {
    return *this = *this / b;
}

big_integer &big_integer::operator %= (const big_integer &b) {
    return *this = *this % b;
}

big_integer &big_integer::operator &= (const big_integer &b) {
    if (size < b.size)
        resize(b.size);
    for (size_t i = 0; i < b.size; ++i)
        data[i] &= b.data[i];
    const uint32_t fill = filler(b.data[b.size - 1]);
    for (size_t i = b.size; i < size; ++i)
        data[i] &= fill;
    normalize();
    return *this;
}

big_integer &big_integer::operator |= (const big_integer &b) {
    if (size < b.size)
        resize(b.size);
    for (size_t i = 0; i < b.size; ++i)
        data[i] |= b.data[i];
    const uint32_t fill = filler(b.data[b.size - 1]);
    for (size_t i = b.size; i < size; ++i)
        data[i] |= fill;
    normalize();
    return *this;
}

big_integer &big_integer::operator ^= (const big_integer &b) {
    if (size < b.size)
        resize(b.size);
    for (size_t i = 0; i < b.size; ++i)
        data[i] ^= b.data[i];
    uint32_t fill = filler(b.data[b.size - 1]);
    for (size_t i = b.size; i < size; ++i)
        data[i] ^= fill;
    normalize();
    return *this;
}

big_integer &big_integer::operator <<= (int b) {
    if (b < 0)
        return *this >>= -b;
    size_t bc = b >> 5, br = b & 31;
    resize(size + bc + 1);
    for (size_t i = size - 1; i != (size_t)-1; --i)
        data[i] = (i >= bc ? data[i - bc] : 0);
    if (br != 0)
        for (size_t i = size - 1; i != (size_t)-1; --i) {
            data[i] <<= br;
            if (i != 0)
                data[i] |= (data[i - 1] >> (32 - br));
        }
    normalize();
    return *this;
}
big_integer &big_integer::operator >>= (int b) {
    if (b < 0)
        return *this <<= -b;
    uint32_t as = filler(data[size - 1]);
    size_t bc = b >> 5, br = b & 31;
    for (size_t i = 0; i < size; ++i)
        data[i] = (i + bc < size ? data[i + bc] : as);
    if (br != 0)
        for (size_t i = 0; i < size; ++i) {
            data[i] >>= br;
            data[i] |= (i + 1 == size ? as : data[i + 1]) << (32 - br);
        }
    normalize();
    return *this;
}

big_integer big_integer::operator + () const {
    return *this;
}

big_integer big_integer::operator - () const {
    big_integer b = ~*this;
    return ++b;
}

big_integer big_integer::operator ~ () const {
    big_integer r;
    r.resize(size);
    for (size_t i = 0; i < size; ++i)
        r.data[i] = ~data[i];
    return r;
}

big_integer &big_integer::operator ++ () {
    return *this += 1;
}

big_integer &big_integer::operator -- () {
    return *this -= 1;
}

big_integer big_integer::operator ++ (int) {
    big_integer a = *this;
    ++*this;
    return a;
}

big_integer big_integer::operator -- (int) {
    big_integer a = *this;
    --*this;
    return a;
}

big_integer operator + (big_integer a, const big_integer &b) {
    //std::cerr << a << " " << b << "\n";
    if (a.size < b.size)
        a.resize(b.size);
    uint32_t carry = 0;
    //  std::cerr << to_string(a) << " " << to_string(b) << "\n";
    uint32_t afill = filler(a.data[a.size - 1]);
    uint32_t bfill = filler(b.data[b.size - 1]);
    //  std::cerr << afill << " " << bfill << "\n";
    for (size_t i = 0; i < b.size; ++i) {
        uint64_t sum = (uint64_t)a.data[i] + b.data[i] + carry;
        a.data[i] = (uint32_t)sum;
        carry = sum >> 32;
    }
    for (size_t i = b.size; i < a.size && bfill + carry; ++i) {
        uint64_t sum = (uint64_t)a.data[i] + bfill + carry;
        a.data[i] = (uint32_t)sum;
        carry = sum >> 32;
    }
    uint32_t newfill = filler(a.data[a.size - 1]);
    //  std::cerr << to_string(a) << "\n";
    if (afill + bfill + carry != newfill) {
        a.resize(a.size + 1);
        a.data[a.size - 1] = afill + bfill + carry;
    }
    a.normalize();
    return a;
}

big_integer operator - (big_integer a, const big_integer &b) {
    if (a.size < b.size)
        a.resize(b.size);
    uint32_t carry = 0;
    uint32_t afill = filler(a.data[a.size - 1]);
    uint32_t bfill = filler(b.data[b.size - 1]);
    for (size_t i = 0; i < b.size; ++i) {
        uint64_t diff = (uint64_t)a.data[i] - b.data[i] - carry;
        a.data[i] = (uint32_t)diff;
        carry = diff >> 63;
    }
    for (size_t i = b.size; i < a.size && bfill + carry; ++i) {
        uint64_t diff = (uint64_t)a.data[i] - bfill - carry;
        a.data[i] = (uint32_t)diff;
        carry = diff >> 63;
    }
    uint32_t newfill = filler(a.data[a.size - 1]);
    if (afill - bfill - carry != newfill) {
        a.resize(a.size + 1);
        a.data[a.size - 1] = afill - bfill - carry;
    }
    a.normalize();
    return a;
}

big_integer operator * (const big_integer &a, const big_integer &b) {
    if (a < 0) {
        if (b < 0)
            return (-a) * (-b);
        else
            return -((-a) * b);
    }
    else if (b < 0)
        return -(a * (-b));
    big_integer r;
    r.resize(a.size + b.size);
    for (size_t i = 0; i < a.size; ++i) {
        uint32_t carry = 0;
        for (size_t j = 0; j < b.size; ++j) {
            uint64_t res = (uint64_t)a.data[i] * b.data[j] + carry + r.data[i + j];
            r.data[i + j] = (uint32_t)res;
            carry = res >> 32;
        }
        for (size_t j = b.size; i + j < r.size && carry; ++j) {
            uint64_t res = (uint64_t)carry + r.data[i + j];
            r.data[i + j] = (uint32_t)res;
            carry = res >> 32;
        }
    }
    r.normalize();
    return r;
}

std::pair <big_integer, big_integer> big_integer::divMod(const big_integer &b) {
    if (*this < b)
        return{ 0, *this };
    if (b.size == 1 || b.size == 2 && b.data[1] == 0) {
        uint32_t carry = 0;
        for (uint32_t i = size; i--; ) {
            uint64_t cur = data[i] + carry * (1LL << 32);
            data[i] = (uint32_t)(cur / b.data[0]);
            carry = (uint32_t)(cur % b.data[0]);
        }
        normalize();
        return{ *this, carry };
    }
    if (b.data[b.size - 1] != 0) {
        int shift = 31 - maxbit(b.data[b.size - 1]);
        auto qr = (*this << shift).divMod(b << shift);
        return{ qr.first, qr.second >> shift };
    }
    //make b proper
    size_t m = size - (data[size - 1] == 0);
    size_t n = b.size - (b.data[b.size - 1] == 0);
    m -= n;
    big_integer q;
    q.resize(m + 2);
    if (*this >= (b << (m * 32))) {
        q.data[m] = 1;
        *this -= (b << (m * 32));
    }
    for (size_t j = m; j--; ) {
        if (n + j >= size)
            continue;
        uint64_t temp = ((static_cast<uint64_t>(data[n + j]) << 32) + data[n + j - 1]) / b.data[n - 1];
        if (temp >> 32) {
            q.data[j] = BASE;
        }
        else {
            q.data[j] = static_cast<uint32_t>(temp);
        }
        *this -= q.data[j] * (b << (j * 32));
        while (*this < 0) {
            --q.data[j];
            *this += (b << (j * 32));
        }
    }
    q.normalize();
    normalize();
    return{ q, *this };
}

big_integer operator / (big_integer a, const big_integer &b) {
    if (a < 0)
        if (b < 0)
            return (-a) / (-b);
        else
            return -((-a) / b);
    else if (b < 0)
        return -(a / (-b));
    return a.divMod(b).first;
}

big_integer operator % (big_integer a, const big_integer &b) {
    if (a < 0)
        if (b < 0)
            return -(-a) % (-b);
        else
            return -((-a) % b);
    else if (b < 0)
        return (a % (-b));
    return a.divMod(b).second;
}

big_integer operator & (big_integer a, const big_integer &b) {
    return a &= b;
}
big_integer operator | (big_integer a, const big_integer &b) {
    return a |= b;
}
big_integer operator ^ (big_integer a, const big_integer &b) {
    return a ^= b;
}

big_integer operator << (big_integer a, int b) {
    return a <<= b;
}

big_integer operator >> (big_integer a, int b) {
    return a >>= b;
}

bool operator == (const big_integer &a, const big_integer &b) {
    for (size_t i = 0; i < std::min(a.size, b.size); ++i)
        if (a.data[i] != b.data[i])
            return false;
    uint32_t afill = filler(a.data[a.size - 1]);
    for (size_t i = a.size; i < b.size; ++i)
        if (afill != b.data[i])
            return false;
    uint32_t bfill = filler(b.data[b.size - 1]);
    for (size_t i = b.size; i < a.size; ++i)
        if (bfill != a.data[i])
            return false;
    return true;
}

bool operator != (const big_integer &a, const big_integer &b) {
    return !(a == b);
}

bool operator < (const big_integer &a, const big_integer &b) {
    uint32_t afill = filler(a.data[a.size - 1]);
    uint32_t bfill = filler(b.data[b.size - 1]);
    if (afill != bfill)
        return afill > bfill;
    for (size_t i = b.size; i-- > a.size;)
        if (afill != b.data[i])
            return afill < b.data[i];
    for (size_t i = a.size; i-- > b.size;)
        if (bfill != a.data[i])
            return a.data[i] < bfill;
    for (size_t i = std::min(a.size, b.size); i--; )
        if (a.data[i] != b.data[i])
            return a.data[i] < b.data[i];
    return false;
}

bool operator > (const big_integer &a, const big_integer &b) {
    return b < a;
}

bool operator <= (const big_integer &a, const big_integer &b) {
    return !(b < a);
}

bool operator >= (const big_integer &a, const big_integer &b) {
    return !(a < b);
}

std::string to_string(big_integer a) {
    if (a == 0)
        return "0";
    std::string s;
    bool neg = false;
    if (a < 0) {
        neg = true;
        a = -a;
    }
    while (a != 0) {
        auto lr = a.divMod(10);
        s.push_back('0' + static_cast<int>(lr.second.data[0]));
        a = lr.first;
    }
    if (neg)
        s.push_back('-');
    std::reverse(s.begin(), s.end());
    return s;
}

std::istream &operator >> (std::istream &in, big_integer &a) {
    std::string s;
    in >> s;
    a = big_integer(s);
    return in;
}

std::ostream &operator << (std::ostream &out, const big_integer &a) {
    return out << to_string(a);
}

void big_integer::resize(size_t nsize) {
    uint32_t * ndata = new uint32_t[nsize];
    std::copy(data, data + std::min(size, nsize), ndata);
    if (nsize > size)
        std::fill(ndata + size, ndata + nsize, filler(data[size - 1]));
    delete[] data;
    size = nsize;
    data = ndata;
}

void big_integer::normalize() {
    uint32_t fill = filler(data[size - 1]);
    for (size_t i = size; i--; )
        if (data[i] != fill || i == 0) {
            if (filler(data[i]) != fill) { //this shouldn't usually happen tho..., happens only if last block was eq to fill and last but one has wrong last bit
                resize(i + 2);
            }
            else {
                resize(i + 1);
            }
            return;
        }
}
