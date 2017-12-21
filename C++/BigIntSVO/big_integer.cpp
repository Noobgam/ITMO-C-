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

static uint32_t* dataAlloc(size_t s)
{
	size_t* data = (size_t*)new uint8_t[sizeof(size_t) + s * sizeof(uint32_t)];
	*data = 0;
	return (uint32_t*)(data + 1);
}


static uint32_t* dataAlloc(size_t s, std::nothrow_t)
{
	size_t* data = (size_t*)new(std::nothrow) uint8_t[sizeof(size_t) + s * sizeof(uint32_t)];
	if (data == nullptr)
		return nullptr;
	*data = 0;
	return (uint32_t*)(data + 1);
}

static void dataRef(uint32_t * x)
{
	size_t* r = (size_t*)x - 1;
	++*r;
}

static size_t refCnt(uint32_t * x)
{
	size_t* r = (size_t*)x - 1;
	return *r;
}

static void dataUnRef(uint32_t * x)
{
	size_t* r = (size_t*)x - 1;
	assert(*r != 0);
	--*r;
	if (*r == 0)
		delete[](uint8_t*)r;
}

uint32_t* big_integer::get_data() const
{
	return size > SMALLSIZE ? dataUnion.data : (uint32_t*)dataUnion.chunk;
}

big_integer::big_integer() :
	big_integer(0) {
}

big_integer::big_integer(const big_integer &b) :
	size(b.size), dataUnion(b.dataUnion)
{
	if (size > SMALLSIZE) {
		dataRef(dataUnion.data);
	}
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
	size(1)
{
	dataUnion.chunk[0] = b;
}

big_integer::big_integer(uint32_t b) {
	dataUnion.chunk[0] = b;
	if (b < (uint32_t)1 << 31) {
		size = 1;
	}
	else {
		size = 2;
		dataUnion.chunk[1] = 0;
	}
}

big_integer::~big_integer() {
	if (size > SMALLSIZE)
		dataUnRef(dataUnion.data);
}

void big_integer::swap(big_integer& b) {
	std::swap(size, b.size);
	std::swap(dataUnion, b.dataUnion);
}

big_integer & big_integer::operator = (big_integer const & b)
{
	if (size > SMALLSIZE)
		dataUnRef(get_data());
	dataUnion = b.dataUnion;
	size = b.size;
	if (size > SMALLSIZE)
		dataRef(get_data());
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
	dupe();
	if (size < b.size)
		resize(b.size);
	for (size_t i = 0; i < b.size; ++i)
		get_data()[i] &= b.get_data()[i];
	const uint32_t fill = filler(b.get_data()[b.size - 1]);
	for (size_t i = b.size; i < size; ++i)
		get_data()[i] &= fill;
	normalize();
	return *this;
}

big_integer &big_integer::operator |= (const big_integer &b) {
	dupe();
	if (size < b.size)
		resize(b.size);
	for (size_t i = 0; i < b.size; ++i)
		get_data()[i] |= b.get_data()[i];
	const uint32_t fill = filler(b.get_data()[b.size - 1]);
	for (size_t i = b.size; i < size; ++i)
		get_data()[i] |= fill;
	normalize();
	return *this;
}

big_integer &big_integer::operator ^= (const big_integer &b) {
	dupe();
	if (size < b.size)
		resize(b.size);
	for (size_t i = 0; i < b.size; ++i)
		get_data()[i] ^= b.get_data()[i];
	uint32_t fill = filler(b.get_data()[b.size - 1]);
	for (size_t i = b.size; i < size; ++i)
		get_data()[i] ^= fill;
	normalize();
	return *this;
}

big_integer &big_integer::operator <<= (int b) {
	dupe();
	if (b < 0)
		return *this >>= -b;
	size_t bc = b >> 5, br = b & 31;
	resize(size + bc + 1);
	for (size_t i = size - 1; i != (size_t)-1; --i)
		get_data()[i] = (i >= bc ? get_data()[i - bc] : 0);
	if (br != 0)
		for (size_t i = size - 1; i != (size_t)-1; --i) {
			get_data()[i] <<= br;
			if (i != 0)
				get_data()[i] |= (get_data()[i - 1] >> (32 - br));
		}
	normalize();
	return *this;
}
big_integer &big_integer::operator >>= (int b) {
	dupe();
	if (b < 0)
		return *this <<= -b;
	uint32_t as = filler(get_data()[size - 1]);
	size_t bc = b >> 5, br = b & 31;
	for (size_t i = 0; i < size; ++i)
		get_data()[i] = (i + bc < size ? get_data()[i + bc] : as);
	if (br != 0)
		for (size_t i = 0; i < size; ++i) {
			get_data()[i] >>= br;
			get_data()[i] |= (i + 1 == size ? as : get_data()[i + 1]) << (32 - br);
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
		r.get_data()[i] = ~(get_data()[i]);
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
	a.dupe();
	//std::cerr << a << " + " << b << "\n";
	if (a.size < b.size)
		a.resize(b.size);
	uint32_t carry = 0;
	//  std::cerr << to_string(a) << " " << to_string(b) << "\n";
	uint32_t afill = filler(a.get_data()[a.size - 1]);
	uint32_t bfill = filler(b.get_data()[b.size - 1]);
	//  std::cerr << afill << " " << bfill << "\n";
	for (size_t i = 0; i < b.size; ++i) {
		uint64_t sum = (uint64_t)a.get_data()[i] + b.get_data()[i] + carry;
		a.get_data()[i] = (uint32_t)sum;
		carry = sum >> 32;
	}
	for (size_t i = b.size; i < a.size && bfill + carry; ++i) {
		uint64_t sum = (uint64_t)a.get_data()[i] + bfill + carry;
		a.get_data()[i] = (uint32_t)sum;
		carry = sum >> 32;
	}
	uint32_t newfill = filler(a.get_data()[a.size - 1]);
	//  std::cerr << to_string(a) << "\n";
	if (afill + bfill + carry != newfill) {
		a.resize(a.size + 1);
		a.get_data()[a.size - 1] = afill + bfill + carry;
	}
	a.normalize();
	return a;
}

big_integer operator - (big_integer a, const big_integer &b) {
	a.dupe();
	if (a.size < b.size)
		a.resize(b.size);
	uint32_t carry = 0;
	uint32_t afill = filler(a.get_data()[a.size - 1]);
	uint32_t bfill = filler(b.get_data()[b.size - 1]);
	for (size_t i = 0; i < b.size; ++i) {
		uint64_t diff = (uint64_t)a.get_data()[i] - b.get_data()[i] - carry;
		a.get_data()[i] = (uint32_t)diff;
		carry = diff >> 63;
	}
	for (size_t i = b.size; i < a.size && bfill + carry; ++i) {
		uint64_t diff = (uint64_t)a.get_data()[i] - bfill - carry;
		a.get_data()[i] = (uint32_t)diff;
		carry = diff >> 63;
	}
	uint32_t newfill = filler(a.get_data()[a.size - 1]);
	if (afill - bfill - carry != newfill) {
		a.resize(a.size + 1);
		a.get_data()[a.size - 1] = afill - bfill - carry;
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
			uint64_t res = (uint64_t)a.get_data()[i] * b.get_data()[j] + carry + r.get_data()[i + j];
			r.get_data()[i + j] = (uint32_t)res;
			carry = res >> 32;
		}
		for (size_t j = b.size; i + j < r.size && carry; ++j) {
			uint64_t res = (uint64_t)carry + r.get_data()[i + j];
			r.get_data()[i + j] = (uint32_t)res;
			carry = res >> 32;
		}
	}
	r.normalize();
	return r;
}

std::pair <big_integer, big_integer> big_integer::divMod(const big_integer &b) {
	if (*this < b)
		return{ 0, *this };
	dupe();
	if (b.size == 1 || b.size == 2 && b.get_data()[1] == 0) {
		uint32_t carry = 0;
		for (uint32_t i = size; i--; ) {
			uint64_t cur = get_data()[i] + carry * (1LL << 32);
			get_data()[i] = (uint32_t)(cur / b.get_data()[0]);
			carry = (uint32_t)(cur % b.get_data()[0]);
		}
		normalize();
		return{ *this, carry };
	}
	if (b.get_data()[b.size - 1] != 0) {
		int shift = 31 - maxbit(b.get_data()[b.size - 1]);
		auto qr = (*this << shift).divMod(b << shift);
		return{ qr.first, qr.second >> shift };
	}
	//make b proper
	size_t m = size - (get_data()[size - 1] == 0);
	size_t n = b.size - (b.get_data()[b.size - 1] == 0);
	m -= n;
	big_integer q;
	q.resize(m + 2);
	if (*this >= (b << (m * 32))) {
		q.get_data()[m] = 1;
		*this -= (b << (m * 32));
	}
	for (size_t j = m; j--; ) {
		if (n + j >= size)
			continue;
		uint64_t temp = ((static_cast<uint64_t>(get_data()[n + j]) << 32) + get_data()[n + j - 1]) / b.get_data()[n - 1];
		if (temp >> 32) {
			q.get_data()[j] = BASE;
		}
		else {
			q.get_data()[j] = static_cast<uint32_t>(temp);
		}
		*this -= q.get_data()[j] * (b << (j * 32));
		while (*this < 0) {
			--q.get_data()[j];
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
	a.dupe();
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
	a.dupe();
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
		if (a.get_data()[i] != b.get_data()[i])
			return false;
	uint32_t afill = filler(a.get_data()[a.size - 1]);
	for (size_t i = a.size; i < b.size; ++i)
		if (afill != b.get_data()[i])
			return false;
	uint32_t bfill = filler(b.get_data()[b.size - 1]);
	for (size_t i = b.size; i < a.size; ++i)
		if (bfill != a.get_data()[i])
			return false;
	return true;
}

bool operator != (const big_integer &a, const big_integer &b) {
	return !(a == b);
}

bool operator < (const big_integer &a, const big_integer &b) {
	uint32_t afill = filler(a.get_data()[a.size - 1]);
	uint32_t bfill = filler(b.get_data()[b.size - 1]);
	if (afill != bfill)
		return afill > bfill;
	for (size_t i = b.size; i-- > a.size;)
		if (afill != b.get_data()[i])
			return afill < b.get_data()[i];
	for (size_t i = a.size; i-- > b.size;)
		if (bfill != a.get_data()[i])
			return a.get_data()[i] < bfill;
	for (size_t i = std::min(a.size, b.size); i--; )
		if (a.get_data()[i] != b.get_data()[i])
			return a.get_data()[i] < b.get_data()[i];
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
		s.push_back('0' + static_cast<int>(lr.second.get_data()[0]));
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
	if (size == nsize)
		return;
	uint32_t * data = get_data();
	uint32_t * ndata;
	if (nsize <= SMALLSIZE) {
		ndata = dataUnion.chunk;
	}
	else if (nsize > size) {
		ndata = dataAlloc(nsize);
	}
	else {
		ndata = dataAlloc(nsize, std::nothrow);
		if (ndata == nullptr)
			return;
	}
	if (data != ndata)
		std::copy(data, data + std::min(size, nsize), ndata);
	if (nsize > size)
		std::fill(ndata + size, ndata + nsize, filler(data[size - 1]));
	if (size > SMALLSIZE)
		dataUnRef(data);
	size = nsize;
	if (size > SMALLSIZE)
	{
		dataRef(ndata);
		dataUnion.data = ndata;
	}
}

void big_integer::dupe() {
	if (size <= SMALLSIZE || refCnt(dataUnion.data) == 1)
		return;
	uint32_t * data = dataUnion.data;
	uint32_t * ndata = dataAlloc(size);
	std::copy(data, data + size, ndata);
	dataUnRef(data);
	dataRef(ndata);
	dataUnion.data = ndata;
}

void big_integer::normalize() {
	uint32_t fill = filler(get_data()[size - 1]);
	for (size_t i = size; i--; )
		if (get_data()[i] != fill || i == 0) {
			if (filler(get_data()[i]) != fill) { //this shouldn't usually happen tho..., happens only if last block was eq to fill and last but one has wrong last bit
				resize(i + 2);
			}
			else {
				resize(i + 1);
			}
			return;
		}
}
