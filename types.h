#pragma once
#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chrono>
#include <sys/stat.h>

#ifdef _WIN32
	#include <direct.h>
	#define MKDIR(x) _mkdir(x)
#else
	#define MKDIR(x) mkdir(x, 0755)
#endif

#ifdef WINVER
#endif
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::microseconds ms;
typedef std::chrono::duration<float> fsec;

#endif