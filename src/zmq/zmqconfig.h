// Copyright (c) 2014 The Bitcoin Core developers
// Copyright (c) 2018-2022 Thought Network Ltd
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef THOUGHT_ZMQ_ZMQCONFIG_H
#define THOUGHT_ZMQ_ZMQCONFIG_H

#if defined(HAVE_CONFIG_H)
#include <config/thought-config.h>
#endif

#include <stdarg.h>
#include <string>

#if ENABLE_ZMQ
#include <zmq.h>
#endif

#include <primitives/block.h>
#include <primitives/transaction.h>

#include <governance/governance-object.h>
#include <governance/governance-vote.h>

#include <llmq/quorums_chainlocks.h>
#include <llmq/quorums_instantsend.h>
#include <llmq/quorums_signing.h>

void zmqError(const char *str);

#endif // THOUGHT_ZMQ_ZMQCONFIG_H
