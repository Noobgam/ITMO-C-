#pragma once

#include <iostream>
#include <string>

class big_integer
{
public:
	big_integer();
	big_integer(const big_integer &b);
	big_integer(int b);
	big_integer(uint32_t b);
	explicit big_integer(std::string const &s);

	~big_integer();

	big_integer& operator=(const big_integer &other);

	big_integer& operator+=(const big_integer &rhs);
	big_integer& operator-=(const big_integer &rhs);
	big_integer& operator*=(const big_integer &rhs);
	big_integer& operator/=(const big_integer &rhs);
	big_integer& operator%=(const big_integer &rhs);

	big_integer& operator&=(const big_integer &rhs);
	big_integer& operator|=(const big_integer &rhs);
	big_integer& operator^=(const big_integer &rhs);

	big_integer& operator<<=(int rhs);
	big_integer& operator>>=(int rhs);

	big_integer operator+() const;
	big_integer operator-() const;
	big_integer operator~() const;

	big_integer& operator++();
	big_integer operator++(int);

	big_integer& operator--();
	big_integer operator--(int);

	friend bool operator==(const big_integer &a, const big_integer &b);
	friend bool operator!=(const big_integer &a, const big_integer &b);
	friend bool operator<(const big_integer &a, const big_integer &b);
	friend bool operator>(const big_integer &a, const big_integer &b);
	friend bool operator<=(const big_integer &a, const big_integer &b);
	friend bool operator>=(const big_integer &a, const big_integer &b);

	friend big_integer operator + (big_integer a, const big_integer &b);
	friend big_integer operator - (big_integer a, const big_integer &b);
	friend big_integer operator * (const big_integer &a, const big_integer &b);
	friend big_integer operator / (big_integer a, const big_integer &b);
	friend big_integer operator % (big_integer a, const big_integer &b);
	friend big_integer operator & (big_integer a, const big_integer &b);
	friend big_integer operator | (big_integer a, const big_integer &b);
	friend big_integer operator ^ (big_integer a, const big_integer &b);
	friend big_integer operator << (big_integer a, int b);
	friend big_integer operator >> (big_integer a, int b);

	friend std::string to_string(big_integer a);

	void swap(big_integer& other);

private:
	size_t size;
	enum { SMALLSIZE = 2 };
	union
	{
		uint32_t* data;
		uint32_t chunk[SMALLSIZE];
	} dataUnion;
	uint32_t* get_data() const;
	void dupe();
	void resize(size_t nsize);
	void normalize();
	std::pair <big_integer, big_integer> divMod(const big_integer &b);
};

big_integer operator + (big_integer a, const big_integer &b);
big_integer operator - (big_integer a, const big_integer &b);
big_integer operator * (const big_integer &a, const big_integer &b);
big_integer operator / (big_integer a, const big_integer &b);
big_integer operator % (big_integer a, const big_integer &b);
big_integer operator & (big_integer a, const big_integer &b);
big_integer operator | (big_integer a, const big_integer &b);
big_integer operator ^ (big_integer a, const big_integer &b);
big_integer operator << (big_integer a, int b);
big_integer operator >> (big_integer a, int b);

bool operator == (const big_integer &a, const big_integer &b);
bool operator != (const big_integer &a, const big_integer &b);
bool operator < (const big_integer &a, const big_integer &b);
bool operator > (const big_integer &a, const big_integer &b);
bool operator <= (const big_integer &a, const big_integer &b);
bool operator >= (const big_integer &a, const big_integer &b);

std::string to_string(big_integer a);
std::istream & operator >> (std::istream &in, big_integer &a);
std::ostream & operator << (std::ostream & out, const big_integer & a);