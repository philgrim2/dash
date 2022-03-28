// Copyright (c) 2013-2018 John Tromp
// Copyright (c) 2019 The Thought Core developers

#ifndef CUCKOOSOLVER_H_
#define CUCKOOSOLVER_H_

#include <vector>
#include <memory>

#include "crypto/cuckoo/cuckoo.h"
#include "consensus/params.h"

namespace cuckoo {

class CContext {
public:
    typedef uint32_t node_t;
    typedef uint32_t edge_t;
    typedef std::pair<node_t, node_t> edge;
    typedef std::vector<uint32_t> solution_t;

    static constexpr unsigned int MAXPATHLEN = 8192;

    CContext(int32_t graphSize);
    ~CContext();

    /// Set the block header to be solved.
    void SetHeader(CBlockHeader const &header);

    /// Get the next solution for the current header, or nullptr if there
    /// isn't any.
    std::unique_ptr<solution_t> GetNextSolution();

private:
    // cuckoo parameters
    uint32_t nnodes;
    uint32_t nedges;
    uint32_t edgemask;
    edge_t easiness;

    /// Keys derived from block header
    siphash_keys sip_keys;

    // cuckoo state
    node_t *cuckoo;
    node_t us[MAXPATHLEN];
    node_t vs[MAXPATHLEN];
    node_t nonce = 0;

    // cuckoo solving requires a slightly different sipnode implementation
    node_t _sipnode(edge_t edge, uint32_t uorv);
    int path(node_t *cuckoo, node_t u, node_t *us);
    std::unique_ptr<solution_t> solution(node_t *us, int nu, node_t *vs, int nv);
};

bool solve(CBlockHeader &header, Consensus::Params const &params);

};

#endif  // CUCKOOSOLVER_H_
