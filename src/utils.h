#pragma once

#include <elf.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

class nullstream : public std::ostream {
public:
    nullstream() : std::ostream(nullptr) {}
};
inline nullstream null;

std::ostream logstream = std::ostream(nullptr);

#define CHECK(x)                                          \
    if (!(x)) {                                           \
        std::cerr << "CHECK failed: " << #x << std::endl; \
        exit(1);                                          \
    }
#define CHECK_GT(x, y) CHECK((x) > (y))
#define CHECK_LT(x, y) CHECK((x) < (y))
#define CHECK_EQ(x, y) CHECK((x) == (y))
#define CHECK_NE(x, y) CHECK((x) != (y))
#define CHECK_GE(x, y) CHECK((x) >= (y))
#define CHECK_LE(x, y) CHECK((x) <= (y))
/* Replace null with std::cout to enable logging */
#define LOG(loglevel)                                                   \
    logstream << std::endl << #loglevel << " " << __FILE__ << ":" << __LINE__ << " " \
              << ""

#define LOG_KEY_VALUE(key, value) " " << key << "=" << value << "\n"
#define LOG_KEY(key) LOG_KEY_VALUE(#key, key)
#define LOG_64BITS(key) LOG_KEY_VALUE(#key, HexString(key, 16))
#define LOG_32BITS(key) LOG_KEY_VALUE(#key, HexString(key, 8))
#define LOG_16BITS(key) LOG_KEY_VALUE(#key, HexString(key, 4))
#define LOG_8BITS(key) LOG_KEY_VALUE(#key, HexString(key, 2))
#define LOG_BITS(key) LOG_KEY_VALUE(#key, HexString(key))
#define LOG_DWEHPE(type) LOG_KEY_VALUE(#type, ShowDW_EH_PE(type))

template <class T>
std::string HexString(T* num, int length = -1) {
    uint64_t n = reinterpret_cast<uint64_t>(num);
    if (length == -1) {
        length = 16;
    }
    std::stringstream ss;
    ss << "0x" << std::uppercase << std::setfill('0') << std::setw(length) << std::hex << n;
    return ss.str();
}

template <class T>
std::string HexString(const T* num, int length = -1) {
    return HexString(const_cast<T*>(num), length);
}

template <class T>
std::string HexString(T num, int length = -1) {
    if (length == -1) {
        length = sizeof(T) / 2;
    }
    std::stringstream ss;
    ss << "0x" << std::uppercase << std::setfill('0') << std::setw(length) << std::hex << +num;
    return ss.str();
}

std::vector<std::string> SplitWith(std::string str, const std::string& delimiter);

std::string ShowRela(const Elf64_Rela& r);

std::string ShowSym(const Elf64_Sym& s, const char* strtab);

std::string ShowRelocationType(int type);
