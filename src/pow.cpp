// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2018-2022 Thought Network Ltd
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <logging.h>
#include <primitives/block.h>
#include <uint256.h>

#include <streams.h>
#include <hash.h>
#include <version.h>
#include <crypto/cuckoo/verify.h>

#include <math.h>

// This is MIDAS (Multi Interval Difficulty Adjustment System), a novel getnextwork algorithm.  It responds quickly to
// huge changes in hashing power, is immune to time warp attacks, and regulates the block rate to keep the block height
// close to the block height expected given the nominal block interval and the elapsed time.  How close the
// correspondence between block height and wall clock time is, depends on how stable the hashing power has been.  Maybe
// Bitcoin can wait 2 weeks between updates but no altcoin can.

// It is important that none of these intervals (5, 7, 9, 17) have any common divisor; eliminating the existence of
// harmonics is an important part of eliminating the effectiveness of timewarp attacks.

void avgRecentTimestamps(const CBlockIndex* pindexLast, int64_t *avgOf5, int64_t *avgOf7, int64_t *avgOf9, int64_t *avgOf17, const Consensus::Params& params)
{
  int blockoffset = 0;
  int64_t oldblocktime;
  int64_t blocktime;

  *avgOf5 = *avgOf7 = *avgOf9 = *avgOf17 = 0;
  if (pindexLast)
    blocktime = pindexLast->GetBlockTime();
  else blocktime = 0;

  for (blockoffset = 0; blockoffset < 17; blockoffset++)
  {
    oldblocktime = blocktime;
    if (pindexLast)
    {
      pindexLast = pindexLast->pprev;
      if (pindexLast)
        blocktime = pindexLast->GetBlockTime();
      else
        blocktime = 0;
    }
    else
    { // genesis block or previous
      blocktime -= params.nPowTargetSpacing;
    }
    // for each block, add interval.
    if (blockoffset < 5) *avgOf5 += (oldblocktime - blocktime);
    if (blockoffset < 7) *avgOf7 += (oldblocktime - blocktime);
    if (blockoffset < 9) *avgOf9 += (oldblocktime - blocktime);
    *avgOf17 += (oldblocktime - blocktime);
  }
  // now we have the sums of the block intervals. Division gets us the averages.
  *avgOf5 /= 5;
  *avgOf7 /= 7;
  *avgOf9 /= 9;
  *avgOf17 /= 17;
}

unsigned int static KimotoGravityWell(const CBlockIndex* pindexLast, const Consensus::Params& params) {
    const CBlockIndex *BlockLastSolved = pindexLast;
    const CBlockIndex *BlockReading = pindexLast;
    uint64_t PastBlocksMass = 0;
    int64_t PastRateActualSeconds = 0;
    int64_t PastRateTargetSeconds = 0;
    double PastRateAdjustmentRatio = double(1);
    arith_uint256 PastDifficultyAverage;
    arith_uint256 PastDifficultyAveragePrev;
    double EventHorizonDeviation;
    double EventHorizonDeviationFast;
    double EventHorizonDeviationSlow;

    uint64_t pastSecondsMin = params.nPowTargetTimespan * 0.025;
    uint64_t pastSecondsMax = params.nPowTargetTimespan * 7;
    uint64_t PastBlocksMin = pastSecondsMin / params.nPowTargetSpacing;
    uint64_t PastBlocksMax = pastSecondsMax / params.nPowTargetSpacing;

    if (BlockLastSolved == nullptr || BlockLastSolved->nHeight == 0 || (uint64_t)BlockLastSolved->nHeight < PastBlocksMin) { return UintToArith256(params.powLimit).GetCompact(); }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) { break; }
        PastBlocksMass++;

        PastDifficultyAverage.SetCompact(BlockReading->nBits);
        if (i > 1) {
            // handle negative arith_uint256
            if(PastDifficultyAverage >= PastDifficultyAveragePrev)
                PastDifficultyAverage = ((PastDifficultyAverage - PastDifficultyAveragePrev) / i) + PastDifficultyAveragePrev;
            else
                PastDifficultyAverage = PastDifficultyAveragePrev - ((PastDifficultyAveragePrev - PastDifficultyAverage) / i);
        }
        PastDifficultyAveragePrev = PastDifficultyAverage;

        PastRateActualSeconds = BlockLastSolved->GetBlockTime() - BlockReading->GetBlockTime();
        PastRateTargetSeconds = params.nPowTargetSpacing * PastBlocksMass;
        PastRateAdjustmentRatio = double(1);
        if (PastRateActualSeconds < 0) { PastRateActualSeconds = 0; }
        if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
            PastRateAdjustmentRatio = double(PastRateTargetSeconds) / double(PastRateActualSeconds);
        }
        EventHorizonDeviation = 1 + (0.7084 * pow((double(PastBlocksMass)/double(28.2)), -1.228));
        EventHorizonDeviationFast = EventHorizonDeviation;
        EventHorizonDeviationSlow = 1 / EventHorizonDeviation;

        if (PastBlocksMass >= PastBlocksMin) {
                if ((PastRateAdjustmentRatio <= EventHorizonDeviationSlow) || (PastRateAdjustmentRatio >= EventHorizonDeviationFast))
                { assert(BlockReading); break; }
        }
        if (BlockReading->pprev == nullptr) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    arith_uint256 bnNew(PastDifficultyAverage);
    if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
        bnNew *= PastRateActualSeconds;
        bnNew /= PastRateTargetSeconds;
    }

    if (bnNew > UintToArith256(params.powLimit)) {
        bnNew = UintToArith256(params.powLimit);
    }

    return bnNew.GetCompact();
}


unsigned int static DarkGravityWave(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params) {
    /* current difficulty formula, thought - DarkGravity v3, written by Evan Duffield - evan@dash.org */
    LogPrint(BCLog::POW, "POW DGW.\n");
    int currentBlockHeight = pindexLast->nHeight+1;
    const arith_uint256 bnPowLimit = (currentBlockHeight >= params.CuckooHardForkBlockHeight)? UintToArith256(params.cuckooPowLimit) : UintToArith256(params.powLimit);

    int64_t nPastBlocks = 24;
    int64_t nLastTimespan = pblock->GetBlockTime() - pindexLast->GetBlockTime();
    // make sure we have at least (nPastBlocks + 1) blocks, otherwise just return powLimit
    if (!pindexLast || pindexLast->nHeight < nPastBlocks) {
        return bnPowLimit.GetCompact();
    }

    if (params.fPowAllowMinDifficultyBlocks) {
        // recent block is more than 2 hours old
        if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + 2 * 60 * 60) {
            LogPrint(BCLog::POW, "DGW mindiffblocks return powlimit >2hrs old.\n");
            return bnPowLimit.GetCompact();
        }
        // recent block is more than 10 minutes old
        if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 4) {
            arith_uint256 bnNew = arith_uint256().SetCompact(pindexLast->nBits) * 10;
            LogPrint(BCLog::POW, "DGW mindiffblocks return powlimit*10 >10min old.\n");
            if (bnNew > bnPowLimit) {
                bnNew = bnPowLimit;
            }
            return bnNew.GetCompact();
        }
    }

    const CBlockIndex *pindex = pindexLast;
    arith_uint256 bnPastTargetAvg;

    for (unsigned int nCountBlocks = 1; nCountBlocks <= nPastBlocks; nCountBlocks++) {
        arith_uint256 bnTarget = arith_uint256().SetCompact(pindex->nBits);

        if (nCountBlocks == 1) {
            bnPastTargetAvg = (bnTarget / nPastBlocks);
        } else {
            // NOTE: that's not an average really...
            bnPastTargetAvg += (bnTarget / nPastBlocks);
        }

        if(nCountBlocks != nPastBlocks) {
            assert(pindex->pprev); // should never fail
            pindex = pindex->pprev;
        }
//    LogPrint(BCLog::POW, "DGW bnTarget: %s bnPartTargetAvg: %s\n", bnTarget.ToString(), bnPastTargetAvg.ToString());
    }

    arith_uint256 bnNew(bnPastTargetAvg);
    LogPrint(BCLog::POW, "DGW PastTargetDiffTotal: %08x nbNew: %08x\n", bnPastTargetAvg.GetCompact(), bnNew.GetCompact());

    // Regulate block times so as to remain synchronized in the long run with the actual time.  The first step is to
    // calculate what interval we want to use as our regulatory goal.  It depends on how far ahead of (or behind)
    // schedule we are.  If we're more than an adjustment period ahead or behind, we use the maximum (nSlowInterval) or minimum
    // (nFastInterval) values; otherwise we calculate a weighted average somewhere in between them.  The closer we are
    // to being exactly on schedule the closer our selected interval will be to our nominal interval (TargetSpacing).

    int64_t nFastInterval = (params.nPowTargetSpacing * 9 ) / 10; // seconds per block desired when far behind schedule
    int64_t nSlowInterval = (params.nPowTargetSpacing * 11) / 10; // seconds per block desired when far ahead of schedule
    int64_t nIntervalDesired  = params.nPowTargetSpacing;
    int64_t then = params.genesisBlockTime;
    int64_t now = pindexLast->GetBlockTime();
    int64_t BlockHeightTime = then + pindexLast->nHeight * params.nPowTargetSpacing;

    if (now < BlockHeightTime + params.DifficultyAdjustmentInterval() && now > BlockHeightTime )
    // ahead of schedule by less than one interval.
    nIntervalDesired = ((params.DifficultyAdjustmentInterval() - (now - BlockHeightTime)) * params.nPowTargetSpacing +
                (now - BlockHeightTime) * nFastInterval) / params.DifficultyAdjustmentInterval();
    else if (now + params.DifficultyAdjustmentInterval() > BlockHeightTime && now < BlockHeightTime)
    // behind schedule by less than one interval.
    nIntervalDesired = ((params.DifficultyAdjustmentInterval() - (BlockHeightTime - now)) * params.nPowTargetSpacing +
                (BlockHeightTime - now) * nSlowInterval) / params.DifficultyAdjustmentInterval();

    // ahead by more than one interval;
    else if (now < BlockHeightTime) nIntervalDesired = nSlowInterval;

    // behind by more than an interval.
    else  nIntervalDesired = nFastInterval;

    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindex->GetBlockTime();
    // NOTE: is this accurate? nActualTimespan counts it for (nPastBlocks - 1) blocks only...
    int64_t nTargetTimespan = nPastBlocks * nIntervalDesired;
    LogPrint(BCLog::POW, "DGW  pre nActualTimespan %d, nTagetTimespan %d, nLastTimespan %d.\n", nActualTimespan, nTargetTimespan, nLastTimespan);
    if (nActualTimespan < nTargetTimespan/3)
        nActualTimespan = nTargetTimespan/3;
    if (nActualTimespan > nTargetTimespan*3)
        nActualTimespan = nTargetTimespan*3;
    LogPrint(BCLog::POW, "DGW  3x over adjust nActualTimespan %d, nTagetTimespan %d.\n", nActualTimespan, nTargetTimespan);
    // Retarget
    LogPrint(BCLog::POW, "DGW  bnNew preadjust: %08x.\n", bnNew.GetCompact());
    bnNew /= nTargetTimespan;
    bnNew *= nActualTimespan;
    LogPrint(BCLog::POW, "DGW  bnNew postadjust: %08x.\n", bnNew.GetCompact());
    if (bnNew > bnPowLimit) {
        bnNew = bnPowLimit;
    }

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequiredBTC(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Only change once per interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 2.5 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 1 day worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

   return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}


//Midas GetNextWorkRequired
unsigned int Midas(const CBlockIndex *pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    int64_t avgOf5;
    int64_t avgOf9;
    int64_t avgOf7;
    int64_t avgOf17;
    int64_t toofast;
    int64_t tooslow;
    int64_t difficultyfactor = 10000;
    int64_t now;
    int64_t BlockHeightTime;

    int64_t nFastInterval = (params.nPowTargetSpacing * 9 ) / 10; // seconds per block desired when far behind schedule
    int64_t nSlowInterval = (params.nPowTargetSpacing * 11) / 10; // seconds per block desired when far ahead of schedule
    int64_t nIntervalDesired  = params.nPowTargetSpacing;


    int currentBlockHeight = pindexLast->nHeight+1;
    const uint256 usedPowLimit = (currentBlockHeight >= params.CuckooHardForkBlockHeight)? params.cuckooPowLimit : params.powLimit;

    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(usedPowLimit).GetCompact();


    if (pindexLast == NULL)
        // Genesis Block
        return nProofOfWorkLimit;

    // Special rule for post-cuckoo fork, so that the difficulty can come down
    // far enough for mining.
      if (currentBlockHeight > params.CuckooHardForkBlockHeight &&
          currentBlockHeight < params.CuckooHardForkBlockHeight + 50)
          {
            return nProofOfWorkLimit;
          }


    if (params.fPowAllowMinDifficultyBlocks)
    {
        LogPrint(BCLog::POW, "Midas POW allowing min difficulty.\n");
        // Special difficulty rule for testnet: If the new block's timestamp is more than 2* TargetSpacing then allow
        // mining of a min-difficulty block.
        if (pblock->nTime > pindexLast->nTime + params.nPowTargetSpacing * 2)
           return nProofOfWorkLimit;
        else
        {
            // Return the last non-special-min-difficulty-rules-block
           const CBlockIndex* pindex = pindexLast;
           while (pindex->pprev && pindex->nHeight % nIntervalDesired != 0 && pindex->nBits == nProofOfWorkLimit)
               pindex = pindex->pprev;
           return pindex->nBits;
        }
    }

    // Regulate block times so as to remain synchronized in the long run with the actual time.  The first step is to
    // calculate what interval we want to use as our regulatory goal.  It depends on how far ahead of (or behind)
    // schedule we are.  If we're more than an adjustment period ahead or behind, we use the maximum (nSlowInterval) or minimum
    // (nFastInterval) values; otherwise we calculate a weighted average somewhere in between them.  The closer we are
    // to being exactly on schedule the closer our selected interval will be to our nominal interval (TargetSpacing).
    //const CBlockIndex* curindex = pindexLast;
    //while (curindex->pprev)
    //           curindex = curindex->pprev;
    //int64_t then = curindex->GetBlockTime();
    int64_t then = params.genesisBlockTime;

    now = pindexLast->GetBlockTime();
    BlockHeightTime = then + pindexLast->nHeight * params.nPowTargetSpacing;

    if (now < BlockHeightTime + params.DifficultyAdjustmentInterval() && now > BlockHeightTime )
    // ahead of schedule by less than one interval.
    nIntervalDesired = ((params.DifficultyAdjustmentInterval() - (now - BlockHeightTime)) * params.nPowTargetSpacing +
                (now - BlockHeightTime) * nFastInterval) / params.DifficultyAdjustmentInterval();
    else if (now + params.DifficultyAdjustmentInterval() > BlockHeightTime && now < BlockHeightTime)
    // behind schedule by less than one interval.
    nIntervalDesired = ((params.DifficultyAdjustmentInterval() - (BlockHeightTime - now)) * params.nPowTargetSpacing +
                (BlockHeightTime - now) * nSlowInterval) / params.DifficultyAdjustmentInterval();

    // ahead by more than one interval;
    else if (now < BlockHeightTime) nIntervalDesired = nSlowInterval;

    // behind by more than an interval.
    else  nIntervalDesired = nFastInterval;

    // find out what average intervals over last 5, 7, 9, and 17 blocks have been.
    avgRecentTimestamps(pindexLast, &avgOf5, &avgOf7, &avgOf9, &avgOf17, params);

    // check for emergency adjustments. These are to bring the diff up or down FAST when a burst miner or multipool
    // jumps on or off.  Once they kick in they can adjust difficulty very rapidly, and they can kick in very rapidly
    // after massive hash power jumps on or off.

    // Important note: This is a self-damping adjustment because 8/5 and 5/8 are closer to 1 than 3/2 and 2/3.  Do not
    // screw with the constants in a way that breaks this relationship.  Even though self-damping, it will usually
    // overshoot slightly. But normal adjustment will handle damping without getting back to emergency.
    toofast = (nIntervalDesired * 2) / 3;
    tooslow = (nIntervalDesired * 3) / 2;

    // both of these check the shortest interval to quickly stop when overshot.  Otherwise first is longer and second shorter.
    if (avgOf5 < toofast && avgOf9 < toofast && avgOf17 < toofast)
    {  //emergency adjustment, slow down (longer intervals because shorter blocks)
    LogPrint(BCLog::POW, "Midas GetNextWorkRequired EMERGENCY RETARGET higher diff lower target\n");
      difficultyfactor *= 8;
      difficultyfactor /= 5;
    }
    else if (avgOf5 > tooslow && avgOf7 > tooslow && avgOf9 > tooslow)
    {  //emergency adjustment, speed up (shorter intervals because longer blocks)
    LogPrint(BCLog::POW, "Midas GetNextWorkRequired EMERGENCY RETARGET lower diff higher target\n");
      difficultyfactor *= 5;
      difficultyfactor /= 8;
    }

    // If no emergency adjustment, check for normal adjustment.
    else if (((avgOf5 > nIntervalDesired || avgOf7 > nIntervalDesired) && avgOf9 > nIntervalDesired && avgOf17 > nIntervalDesired) ||
         ((avgOf5 < nIntervalDesired || avgOf7 < nIntervalDesired) && avgOf9 < nIntervalDesired && avgOf17 < nIntervalDesired))
    { // At least 3 averages too high or at least 3 too low, including the two longest. This will be executed 3/16 of
      // the time on the basis of random variation, even if the settings are perfect. It regulates one-sixth of the way
      // to the calculated point.
      LogPrint(BCLog::POW, "Midas GetNextWorkRequired RETARGET\n");
      difficultyfactor *= (6 * nIntervalDesired);
      difficultyfactor /= avgOf17 +(5 * nIntervalDesired);
    }

    // limit to doubling or halving.  There are no conditions where this will make a difference unless there is an
    // unsuspected bug in the above code.
    if (difficultyfactor > 20000) difficultyfactor = 20000;
    if (difficultyfactor < 5000) difficultyfactor = 5000;

    arith_uint256 bnNew;
    arith_uint256 bnOld;

    bnOld.SetCompact(pindexLast->nBits);

    if (difficultyfactor == 10000) // no adjustment.
      return(bnOld.GetCompact());

    bnNew = bnOld / difficultyfactor;
    bnNew *= 10000;

    // cuckoo related
    const arith_uint256 bnPowLimit = UintToArith256(usedPowLimit);

    /*
    //added for no cuckoo
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    */

    if (bnNew > bnPowLimit)
      bnNew = bnPowLimit;

    LogPrint(BCLog::POW, "Midas Actual time %d, Scheduled time for this block height = %d\n", now, BlockHeightTime );
    LogPrint(BCLog::POW, "Midas Nominal block interval = %d, regulating on interval %d to get back to schedule.\n", params.nPowTargetSpacing, nIntervalDesired );
    LogPrint(BCLog::POW, "Midas Intervals of last 5/7/9/17 blocks = %d / %d / %d / %d.\n", avgOf5, avgOf7, avgOf9, avgOf17);
    LogPrint(BCLog::POW, "Midas Difficulty Before Adjustment: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
    LogPrint(BCLog::POW, "Midas Difficulty After Adjustment:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

    return bnNew.GetCompact();
}


unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    assert(pblock != nullptr);

    // this is only active on devnets
    int currentBlockHeight = pindexLast->nHeight+1;
    if (pindexLast->nHeight < params.nMinimumDifficultyBlocks) {
        unsigned int nProofOfWorkLimit = (currentBlockHeight >= params.CuckooHardForkBlockHeight)? UintToArith256(params.cuckooPowLimit).GetCompact() : UintToArith256(params.powLimit).GetCompact();
        return nProofOfWorkLimit;
    }

    // Most recent algo first
    if (pindexLast->nHeight + 1 >= params.nPowDGWHeight) {
        return DarkGravityWave(pindexLast, pblock, params);
    }
    else if (pindexLast->nHeight + 1 >= params.midasStartHeight) {
        return Midas(pindexLast, pblock, params);
    }
    else {
        return GetNextWorkRequiredBTC(pindexLast, pblock, params);
    }
}

// for DIFF_BTC only!
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

// CheckProofOfWork SHA and Cuckoo
bool CheckProofOfWork(const CBlockHeader& blockHeader, uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bool retval = true;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);
    LogPrint(BCLog::POW, "CheckPOW Checking against target: %s\n", bnTarget.GetHex());

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow)
    {
        retval = false;
    }
        else
    {
        if (blockHeader.isCuckooPow())
        {
            if (bnTarget > UintToArith256(params.cuckooPowLimit)
                || !CheckCuckooProofOfWork(blockHeader, params))
            {
                retval = false;
            }
            else
            {
                const uint32_t *proof = blockHeader.cuckooProof;
                unsigned char cuckooHash[32];

                CHash256().Write((const unsigned char*)proof, 42 * sizeof(uint32_t)).Finalize(cuckooHash);

                std::vector<unsigned char> vec;
                for (int i = 0; i < 32; i++)
                {
                    vec.push_back(cuckooHash[i]);
                }

                vec.resize(32);
                arith_uint256 cpow = UintToArith256((const uint256)vec);
                LogPrint(BCLog::POW, "Cuckoo Difficulty In: %s\n", cpow.GetHex());

                if (cpow > bnTarget)
                {
                    LogPrintf("Cuckoo POW Hash over target.");
                    retval = false;
                }
            }
        }
        else
        {
            // Check proof of work matches claimed amount
            if (UintToArith256(blockHeader.GetHash()) > bnTarget
                || bnTarget > UintToArith256(params.powLimit))
            {
                retval = false;
            }
        }
    }
    return retval;
}

bool CheckCuckooProofOfWork(const CBlockHeader& blockHeader, const Consensus::Params& params) {
    // Serialize header and trim to 80 bytes
    bool retval = false;

    unsigned char hash[32];
    cuckoo::hash_blockheader(blockHeader, hash);

    // Check for valid cuckoo cycle
    auto vc = cuckoo::verify((unsigned int *)blockHeader.cuckooProof, hash, params.cuckooGraphSize);
    if (cuckoo::POW_OK == vc)
    {
      LogPrint(BCLog::POW, "Cuckoo cycle verified!\n");
      retval = true;
    }
    else
    {
      LogPrint(BCLog::POW, "Cuckoo cycle not verified, code %d\n", vc);
    }

    return retval;
}
