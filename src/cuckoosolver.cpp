// Copyright (c) 2013-2016 John Tromp
// Copyright (c) 2019 The Thought Core developers


#include <set>
#include <cstdlib>
#include <cstring>

#include "cuckoosolver.h"
#include "pow.h"

namespace cuckoo {


CContext::CContext(int32_t graphSize)
{
    nedges = ((uint32_t)1 << (graphSize - 1));
    nnodes = 2 * nedges;
    edgemask = nedges - 1;
    easiness = nnodes;
    cuckoo = new node_t[1 + nnodes];
}


CContext::~CContext()
{
    delete[] cuckoo;
}


CContext::node_t CContext::_sipnode(CContext::edge_t edge, uint32_t uorv)
{
    return siphash24(&sip_keys, 2*edge + uorv) & edgemask;
}


int CContext::path(node_t *cuckoo, node_t u, node_t *us)
{
    int nu;
    for (nu = 0; u; u = cuckoo[u]) {
        if (++nu >= MAXPATHLEN) {
            while (nu-- && us[nu] != u) ;
            if (nu < 0)
                throw std::runtime_error("maximum path length exceeded");
            else {
                printf("illegal % 4d-cycle\n", MAXPATHLEN-nu);
                throw std::runtime_error("illegal cycle");
            }
        }
        us[nu] = u;
    }
    return nu;
}


std::unique_ptr<CContext::solution_t> CContext::solution(node_t *us, int nu, node_t *vs, int nv)
{
    std::set<edge> cycle;
    unsigned n;
    cycle.insert(edge(*us, *vs));
    auto nonces = std::make_unique<solution_t>();
    nonces->reserve(42);

    while (nu--)
        cycle.insert(edge(us[(nu+1)&~1], us[nu|1])); // u's in even position; v's in odd
    while (nv--)
        cycle.insert(edge(vs[nv|1], vs[(nv+1)&~1])); // u's in odd position; v's in even

    unsigned int found = 0;
    for (edge_t nonce_ = n = 0; nonce_ < easiness; nonce_++) {
        edge e(2*_sipnode(nonce_, 0), 2*_sipnode(nonce_, 1)+1);
        if (cycle.find(e) != cycle.end()) {
            nonces->emplace_back(nonce_);
            found++;
            // CContext::GetNextSolution already checks the length, so we can return early
            if (found == PROOFSIZE) {
                return nonces;
            }
            cycle.erase(e);
        }
    }
    return nonces;
}


void CContext::SetHeader(CBlockHeader const &header)
{
    unsigned char hash[32];
    hash_blockheader(header, hash);
    siphash_setkeys(&sip_keys, hash);
    std::memset(cuckoo, 0, sizeof(node_t) * (1 + nnodes));
    nonce = 0;
}


std::unique_ptr<CContext::solution_t> CContext::GetNextSolution()
{
    for (; nonce < easiness; nonce++) {
        node_t u0 = 2*_sipnode(nonce, 0);
        if (u0 == 0) continue; // reserve 0 as nil; v0 guaranteed non-zero
        node_t v0 = 2*_sipnode(nonce, 1)+1;
        node_t u = cuckoo[u0], v = cuckoo[v0];
        us[0] = u0;
        vs[0] = v0;

        int nu = path(cuckoo, u, us), nv = path(cuckoo, v, vs);
        if (us[nu] == vs[nv]) {
            int min = nu < nv ? nu : nv;
            for (nu -= min, nv -= min; us[nu] != vs[nv]; nu++, nv++) ;
            int len = nu + nv + 1;
            if (len == PROOFSIZE)
                return std::move(solution(us, nu, vs, nv));
            continue;
        }
        if (nu < nv) {
            while (nu--)
                cuckoo[us[nu+1]] = us[nu];
            cuckoo[u0] = v0;
        } else {
            while (nv--)
                cuckoo[vs[nv+1]] = vs[nv];
            cuckoo[v0] = u0;
        }
    }
    return nullptr;
}

bool solve(CBlockHeader &header, Consensus::Params const &params)
{
    CContext ctx(params.cuckooGraphSize);

    for (header.nNonce == 0; header.nNonce <= std::numeric_limits<uint32_t>::max(); header.nNonce += 1) {
        ctx.SetHeader(header);

        // while (auto sol = ctx.GetNextSolution()) {
        if (auto sol = ctx.GetNextSolution()) {
            assert(sol->size() == PROOFSIZE);
            std::memcpy(header.cuckooProof, sol->data(), sizeof(uint32_t) * PROOFSIZE);
            if (CheckProofOfWork(header, header.GetHash(), header.nBits, params)) {
                return true;
            }
        }
    }
    return false;
}

};
