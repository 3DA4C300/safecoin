// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "checkpoints.h"

#include "chainparams.h"
#include "main.h"
#include "uint256.h"

#include <stdint.h>

namespace Checkpoints {

    /**
     * How many times slower we expect checking transactions after the last
     * checkpoint to be (from checking signatures, which is skipped up to the
     * last checkpoint). This number is a compromise, as it can't be accurate
     * for every system. When reindexing from a fast disk with a slow CPU, it
     * can be up to 20, while when downloading from a slow network with a
     * fast multicore CPU, it won't be much higher than 1.
     */
    static const double SIGCHECK_VERIFICATION_FACTOR = 5.0;
    bool CheckBlock(const CChainParams::CCheckpointData& data, int nHeight, const uint256& hash)
    {
        const MapCheckpoints& checkpoints = data.mapCheckpoints;
        
        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    //! Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(const CChainParams::CCheckpointData& data, CBlockIndex *pindex, bool fSigchecks) {
        if (pindex==NULL)
            return 0.0;

        int64_t nNow = time(NULL);

        double fSigcheckVerificationFactor = fSigchecks ? SIGCHECK_VERIFICATION_FACTOR : 1.0;
        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkpoint, and
        // fSigcheckVerificationFactor per transaction after.

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->GetBlockTime())/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    int GetTotalBlocksEstimate(const CChainParams::CCheckpointData& data)
    {
        const MapCheckpoints& checkpoints = data.mapCheckpoints;

        if (checkpoints.empty())
            return 0;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const CChainParams::CCheckpointData& data)
    {
        const MapCheckpoints& checkpoints = data.mapCheckpoints;

        for (auto it = checkpoints.rbegin(); it != checkpoints.rend(); it++) {
            const uint256& hash = it->second;
            BlockMap::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }

} // namespace Checkpoints
