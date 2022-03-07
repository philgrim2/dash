// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2018-2022 Thought Network Ltd
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef THOUGHT_POLICY_FEERATE_H
#define THOUGHT_POLICY_FEERATE_H

#include <amount.h>
#include <serialize.h>

#include <string>

extern const std::string CURRENCY_UNIT;

/**
 * Fee rate in notions per kilobyte: CAmount / kB
 */
class CFeeRate
{
private:
    CAmount nNotionsPerK; // unit is notions-per-1,000-bytes

public:
    /** Fee rate of 0 notions per kB */
    CFeeRate() : nNotionsPerK(0) { }
    template<typename I>
    CFeeRate(const I _nNotionsPerK): nNotionsPerK(_nNotionsPerK) {
        // We've previously had bugs creep in from silent double->int conversion...
        static_assert(std::is_integral<I>::value, "CFeeRate should be used without floats");
    }
    /** Constructor for a fee rate in notions per kB. The size in bytes must not exceed (2^63 - 1)*/
    CFeeRate(const CAmount& nFeePaid, size_t nBytes);
    /**
     * Return the fee in notions for the given size in bytes.
     */
    CAmount GetFee(size_t nBytes) const;
    /**
     * Return the fee in notions for a size of 1000 bytes
     */
    CAmount GetFeePerK() const { return GetFee(1000); }
    friend bool operator<(const CFeeRate& a, const CFeeRate& b) { return a.nNotionsPerK < b.nNotionsPerK; }
    friend bool operator>(const CFeeRate& a, const CFeeRate& b) { return a.nNotionsPerK > b.nNotionsPerK; }
    friend bool operator==(const CFeeRate& a, const CFeeRate& b) { return a.nNotionsPerK == b.nNotionsPerK; }
    friend bool operator<=(const CFeeRate& a, const CFeeRate& b) { return a.nNotionsPerK <= b.nNotionsPerK; }
    friend bool operator>=(const CFeeRate& a, const CFeeRate& b) { return a.nNotionsPerK >= b.nNotionsPerK; }
    friend bool operator!=(const CFeeRate& a, const CFeeRate& b) { return a.nNotionsPerK != b.nNotionsPerK; }
    CFeeRate& operator+=(const CFeeRate& a) { nNotionsPerK += a.nNotionsPerK; return *this; }
    std::string ToString() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nNotionsPerK);
    }
};

#endif //  THOUGHT_POLICY_FEERATE_H
