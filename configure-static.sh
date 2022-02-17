#!/bin/bash
# Copyright (c) 2017-2022 Thought Network Ltd
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set -e
set -x

BASEDIR=$(pwd)

if [ "x${HOST}y" = "xy" ]; then
	export HOST="$(./depends/config.guess)"
fi

(cd depends && make)

./autogen.sh

export CONFIG_SITE="$BASEDIR/depends/$HOST/share/config.site"
./configure --prefix=/ $@
