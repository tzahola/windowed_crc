//
//  main.cpp
//  CRCSample
//
//  Created by Tamás Zahola on 2020. 05. 16..
//  Copyright © 2020. Tamás Zahola. All rights reserved.
//

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>


template<typename T>
struct put_poly_t: std::enable_if<std::is_unsigned<T>::value> {
    T poly;
};

template<typename T>
static put_poly_t<T> put_poly(T poly) {
    put_poly_t<T> p;
    p.poly = poly;
    return p;
}

template<typename T>
static std::ostream& operator<<(std::ostream& os, put_poly_t<T> p) {
    bool monomial = true;
    unsigned i = sizeof(p.poly) * 8 - 1;
    while (true) {
        if ((p.poly >> (sizeof(p.poly) * 8 - 1 - i)) & 1) {
            if (monomial) {
                monomial = false;
            } else {
                os << " + ";
            }

            if (i == 0) {
                os << "1";
            } else if (i == 1) {
                os << "x";
            } else {
                os << "x^" << i;
            }
        }
        if (i == 0){
            break;
        } else {
            i--;
        }
    }
    if (monomial) {
        os << "0";
    }
    return os;
}

static std::uint32_t mul(std::uint32_t a, std::uint32_t b, std::uint32_t poly) {
    std::uint32_t result = 0;
    
    while (a != 0) {
        if (a & (1 << 31)) {
            result ^= b;
        }
        b = (b >> 1) ^ ((b & 1) ? poly : 0);
        a <<= 1;
    }
    return result;
}

static std::vector<std::uint32_t> makeTable(std::uint32_t unit, std::uint32_t poly) {
    std::vector<std::uint32_t> table(1 << 8);
    table[0] = 0;

    for (unsigned i = 1 << 7; i != 0; i >>= 1) {
        table[i] = unit;
        unit = (unit >> 1) ^ ((unit & 1) ? poly : 0);
    }

    for (unsigned i = 2; i != table.size(); i <<= 1) {
        for (unsigned j = 1; j != i; j++) {
            table[i ^ j] = table[i] ^ table[j];
        }
    }

    return table;
}

static unsigned clzl(unsigned long x) {
    static_assert(__has_builtin(__builtin_clzl), "");
    return __builtin_clzl(x);
}

int main(int argc, const char * argv[]) {
    unsigned long window = 0;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--window") {
            i++;
            if (i < argc) {
                window = std::atoi(argv[i]);
            }
        } else {
            std::cerr << "Error: unknown switch: " << argv[i] << std::endl;
            return 3;
        }
    }
    if (window < 1) {
        std::cerr << "Error: need a positive window size via --window" << std::endl;
        return 3;
    }

    const std::uint32_t preXOR = ~0;
    const std::uint32_t postXOR = ~0;
    const std::uint32_t poly = 0xEDB88320; // (x^32 +) x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1

    auto crcTable = makeTable(poly, poly);

    std::vector<std::uint8_t> buffer(window);
    if(!std::cin.read(reinterpret_cast<char*>(buffer.data()), buffer.size())) {
        if (std::cin.bad()) {
            std::cerr << "Error: failed to read input" << std::endl;
            return 1;
        } else {
            std::cerr << "Error: less input than window length" << std::endl;
            return 2;
        }
    }

    std::uint32_t crc = 0;
    for (long i = 0; i != window; i++) {
        crc = crcTable[(buffer[i] ^ crc) & 0xFF] ^ (crc >> 8);
    }

    std::uint32_t xPowWindowBits = 1 << 31;
    {
        unsigned long windowBits = 8 * window;
        for (unsigned long i = 1 << (sizeof(windowBits) * 8 - clzl(windowBits)); i != 0; i >>= 1) {
            xPowWindowBits = mul(xPowWindowBits, xPowWindowBits, poly);
            if (windowBits & i) {
                xPowWindowBits = (xPowWindowBits >> 1) ^ (xPowWindowBits & 1 ? poly : 0);
            }
        }
    }

    std::uint32_t crcXOR = mul(preXOR, xPowWindowBits, poly) ^ postXOR;
    auto prefixByteTable = makeTable(mul(poly, xPowWindowBits, poly), poly);

    while (true) {
        for (long i = 0; i != window; i++) {
            std::cout << std::hex << std::setw(8) << std::setfill('0') << (crc ^ crcXOR) << std::endl;

            std::uint8_t byte;
            if (!std::cin.read(reinterpret_cast<char*>(&byte), 1)) {
                if (std::cin.eof()) {
                    return 0;
                } else {
                    std::cerr << "Error: failed to read input" << std::endl;
                    return 1;
                }
            }
            crc = crcTable[(byte ^ crc) & 0xFF] ^ (crc >> 8) ^ prefixByteTable[buffer[i]];
            buffer[i] = byte;
        }
    }

    assert(false); // unreachable
    return -1;
}
