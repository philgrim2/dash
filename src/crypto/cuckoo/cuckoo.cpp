// Copyright (c) 2013-2016 John Tromp
// Copyright (c) 2018-2021 The Thought Core developers

#include "crypto/cuckoo/cuckoo.h"
#include "crypto/sha256.h"
#include "compat/endian.h"
#include "streams.h"
#include "version.h"

#ifndef ROTL
#define ROTL(x,b) (uint64_t)( ((x) << (b)) | ( (x) >> (64 - (b))) )
#endif
#ifndef SIPROUND
#define SIPROUND \
  do { \
    v0 += v1; v2 += v3; v1 = ROTL(v1,13); \
    v3 = ROTL(v3,16); v1 ^= v0; v3 ^= v2; \
    v0 = ROTL(v0,32); v2 += v1; v0 += v3; \
    v1 = ROTL(v1,17);   v3 = ROTL(v3,21); \
    v1 ^= v2; v3 ^= v0; v2 = ROTL(v2,32); \
  } while(0)
#endif

namespace cuckoo {

uint64_t siphash24(const siphash_keys *keys, const uint64_t nonce)
{
    uint64_t v0 = keys->k0, v1 = keys->k1, v2 = keys->k2, v3 = keys->k3 ^ nonce;
    SIPROUND; SIPROUND;
    v0 ^= nonce;
    v2 ^= 0xff;
    SIPROUND; SIPROUND; SIPROUND; SIPROUND;
    return (v0 ^ v1) ^ (v2  ^ v3);
}

void siphash_setkeys(siphash_keys *keys, const unsigned char *keybuf)
{
    keys->k0 = htole64(((uint64_t *)keybuf)[0]);
    keys->k1 = htole64(((uint64_t *)keybuf)[1]);
    keys->k2 = htole64(((uint64_t *)keybuf)[2]);
    keys->k3 = htole64(((uint64_t *)keybuf)[3]);
}

uint32_t sipnode(siphash_keys const *keys, uint32_t nonce, uint32_t uorv, uint32_t edgemask)
{
    return (siphash24(keys, 2*nonce + uorv) & edgemask) << 1 | uorv;
}


void hash_blockheader(CBlockHeader const &header, unsigned char hash[32])
{
    std::vector<unsigned char> serializedHeader;
    CVectorWriter(SER_NETWORK, INIT_PROTO_VERSION, serializedHeader, 0, header);
    serializedHeader.resize(HEADERSIZE);

    CSHA256().Write((const unsigned char *)serializedHeader.data(), HEADERSIZE).Finalize(hash);
}

};
