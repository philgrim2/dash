// Copyright (c) 2015 The Bitcoin Core developers
// Copyright (c) 2018-2022 Thought Network Ltd
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Functionality for communicating with Tor.
 */
#ifndef THOUGHT_TORCONTROL_H
#define THOUGHT_TORCONTROL_H

#include <scheduler.h>

extern const std::string DEFAULT_TOR_CONTROL;
static const bool DEFAULT_LISTEN_ONION = true;

void StartTorControl();
void InterruptTorControl();
void StopTorControl();

#endif /* THOUGHT_TORCONTROL_H */
