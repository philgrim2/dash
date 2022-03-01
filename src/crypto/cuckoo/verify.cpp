// Copyright (c) 2013-2016 John Tromp
// Copyright (c) 2018-2021 The Thought Core developers

#include "crypto/cuckoo/verify.h"

namespace cuckoo {

verify_code verify(uint32_t nonces[PROOFSIZE], const unsigned char *buf, uint32_t graphSize)
{
    uint32_t nnodes = ((uint32_t)1 << graphSize);
    uint32_t nedges = ((uint32_t)1 << (graphSize - 1));
    uint32_t edgemask =  ((uint32_t)nedges - 1);

    siphash_keys keys;
    siphash_setkeys(&keys, buf);
    uint32_t uvs[2*PROOFSIZE];
    uint32_t xor0=0,xor1=0;
    for (uint32_t n = 0; n < PROOFSIZE; n++) {
        if (nonces[n] > nnodes)
            return POW_TOO_BIG;
        if (n && nonces[n] <= nonces[n-1])
            return POW_TOO_SMALL;
    xor0 ^= uvs[2 * n] = sipnode(&keys, nonces[n], 0, edgemask);
    xor1 ^= uvs[2 * n + 1] = sipnode(&keys, nonces[n], 1, edgemask);
    }
    if (xor0|xor1)              // matching endpoints imply zero xors
        return POW_NON_MATCHING;
    uint32_t n = 0, i = 0, j;
    do {                        // follow cycle
        for (uint32_t k = j = i; (k = (k+2) % (2*PROOFSIZE)) != i; ) {
            if (uvs[k] == uvs[i]) { // find other edge endpoint identical to one at i
                if (j != i)           // already found one before
                    return POW_BRANCH;
                j = k;
            }
        }
        if (j == i) return POW_DEAD_END;  // no matching endpoint
        i = j^1;
        n++;
    } while (i != 0);           // must cycle back to start or we would have found branch
    return n == PROOFSIZE ? POW_OK : POW_SHORT_CYCLE;
}

};
