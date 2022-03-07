// Copyright (c) 2014-2021 The Dash Core developers
// Copyright (c) 2018-2022 Thought Network Ltd
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef THOUGHT_MASTERNODE_MASTERNODE_UTILS_H
#define THOUGHT_MASTERNODE_MASTERNODE_UTILS_H

#include <evo/deterministicmns.h>

class CConnman;

class CMasternodeUtils
{
public:
    static void ProcessMasternodeConnections(CConnman& connman);
    static void DoMaintenance(CConnman &connman);
};

#endif // THOUGHT_MASTERNODE_MASTERNODE_UTILS_H
