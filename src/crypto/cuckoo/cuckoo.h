/*
     The MIT License (MIT)

    Copyright (c) 2013-2019 John Tromp

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef CUCKOO_H_
#define CUCKOO_H_

#include <cstdint>
#include <cstddef>

#include "primitives/block.h"

/// Cuckoo cycle-related functions and classes.
namespace cuckoo {

typedef struct
{
    uint64_t k0;
    uint64_t k1;
    uint64_t k2;
    uint64_t k3;
} siphash_keys;

typedef uint32_t word_t;

/// Number of nonces in a cuckoo proof.
constexpr size_t PROOFSIZE = 42;

/// Size in bytes of the header blob used to generate siphash keys.
constexpr size_t HEADERSIZE = 80;

uint32_t sipnode(siphash_keys const *keys, uint32_t nonce, uint32_t uorv, uint32_t edgemask);

/// set siphash keys from 64 byte char array
void siphash_setkeys(siphash_keys *keys, unsigned char const *keybuf);

/// SipHash-2-4 specialized to precomputed key and 8 byte nonces
uint64_t siphash24(siphash_keys const *keys, const uint64_t nonce);

/// Hashes the given block header for use as a siphash_keys structure
void hash_blockheader(CBlockHeader const &header, unsigned char hash[32]);

};

#endif  // CUCKOO_H_
