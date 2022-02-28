// Copyright (c) 2017 The Bitcoin Core developers
// Copyright (c) 2018-2022 Thought Network Ltd
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef THOUGHT_RPC_RAWTRANSACTION_H
#define THOUGHT_RPC_RAWTRANSACTION_H

class CBasicKeyStore;
struct CMutableTransaction;
class UniValue;

/** Sign a transaction with the given keystore and previous transactions */
UniValue SignTransaction(CMutableTransaction& mtx, const UniValue& prevTxs, CBasicKeyStore *keystore, bool tempKeystore, const UniValue& hashType);

#endif // THOUGHT_RPC_RAWTRANSACTION_H
