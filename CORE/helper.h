#pragma once

#include <iostream>

typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef float float32;



inline uint32
writeInt8(int8 tmp, std::ostream &rw)
{
	rw.write(reinterpret_cast <char *> (&tmp), sizeof(int8));
	return sizeof(int8);
}

inline uint32
writeUInt8(uint8 tmp, std::ostream &rw)
{
	rw.write(reinterpret_cast <char *> (&tmp), sizeof(uint8));
	return sizeof(uint8);
}

inline uint32
writeInt16(int16 tmp, std::ostream &rw)
{
	rw.write(reinterpret_cast <char *> (&tmp), sizeof(int16));
	return sizeof(int16);
}

inline uint32
writeUInt16(uint16 tmp, std::ostream &rw)
{
	rw.write(reinterpret_cast <char *> (&tmp), sizeof(uint16));
	return sizeof(uint16);
}

inline uint32
writeInt32(int32 tmp, std::ostream &rw)
{
	rw.write(reinterpret_cast <char *> (&tmp), sizeof(int32));
	return sizeof(int32);
}

inline uint32
writeUInt32(uint32 tmp, std::ostream &rw)
{
	rw.write(reinterpret_cast <char *> (&tmp), sizeof(uint32));
	return sizeof(uint32);
}

inline uint32
writeFloat32(float32 tmp, std::ostream &rw)
{
	rw.write(reinterpret_cast <char *> (&tmp), sizeof(float32));
	return sizeof(float32);
}

inline int8
readInt8(std::istream &rw)
{
	int8 tmp;
	rw.read(reinterpret_cast <char *> (&tmp), sizeof(int8));
	return tmp;
}

inline uint8
readUInt8(std::istream &rw)
{
	uint8 tmp;
	rw.read(reinterpret_cast <char *> (&tmp), sizeof(uint8));
	return tmp;
}

inline int16
readInt16(std::istream &rw)
{
	int16 tmp;
	rw.read(reinterpret_cast <char *> (&tmp), sizeof(int16));
	return tmp;
}

inline uint16
readUInt16(std::istream &rw)
{
	uint16 tmp;
	rw.read(reinterpret_cast <char *> (&tmp), sizeof(uint16));
	return tmp;
}

inline int32
readInt32(std::istream &rw)
{
	int32 tmp;
	rw.read(reinterpret_cast <char *> (&tmp), sizeof(int32));
	return tmp;
}

inline uint32
readUInt32(std::istream &rw)
{
	uint32 tmp;
	rw.read(reinterpret_cast <char *> (&tmp), sizeof(uint32));
	return tmp;
}

inline float32
readFloat32(std::istream &rw)
{
	float32 tmp;
	rw.read(reinterpret_cast <char *> (&tmp), sizeof(float32));
	return tmp;
}