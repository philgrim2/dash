#!/usr/bin/env bash
# use testnet settings,  if you need mainnet,  use ~/.thoughtcore/thoughtd.pid file instead
export LC_ALL=C

thought_pid=$(<~/.thoughtcore/testnet3/thoughtd.pid)
sudo gdb -batch -ex "source debug.gdb" thoughtd ${thought_pid}
