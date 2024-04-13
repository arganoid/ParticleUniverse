#pragma once

#include <string>
#include <optional>
#include <fstream>
#include <sstream>

inline int getrandom(int min, int max)
{
	return (rand() % ((max + 1) - min)) + min;
}

inline float FRand(float minVal, float maxVal)
{
	return minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
}

void argDebug(std::string const& _str);
void argDebugf(std::string _str, ...);

std::string stringFormat(std::string _str, ...);

template<typename T> void writeByte(std::ofstream& file, T data)
{
	const __int8 byte = data;
	if (!file.is_open())
	{
		std::ostringstream s;
		s << "File not open for writing - data: " << data;
		throw std::runtime_error(s.str());
	}
	file.write(&byte, sizeof(decltype(byte)));
}

template<typename T> void readByte(std::ifstream& file, T& data)
{
	if (!file.is_open())
		throw std::runtime_error("File not open for reading");
	__int8 byte;
	file.read(&byte, sizeof(decltype(byte)));
	data = static_cast<T>(byte);
}

template<typename T> void write(std::ofstream& file, T data)
{
	if (!file.is_open())
	{
		std::ostringstream s;
		s << "File not open for writing - data: " << data;
		throw std::runtime_error(s.str());
	}
	file.write(reinterpret_cast<const char*>(&data), sizeof(decltype(data)));
}

template<typename T> void write(std::ofstream& file, std::optional<T> data)
{
	write(file, data.value());
}

template<typename T> void read(std::ifstream& file, T& data)
{
	if (!file.is_open())
		throw std::runtime_error("File not open for reading");
	file.read(reinterpret_cast<char*>(&data), sizeof(decltype(data)));
}

template<typename T> void read(std::ifstream& file, std::optional<T>& data)
{
	T val;
	read(file, val);
	data = val;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, std::optional<T> const& o)
{
	if (!o.has_value())
		os << "nullopt";
	else
		os << o.value();
	return os;
}
