#!/usr/bin/env bash

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

THOUGHTD=${THOUGHTD:-$BINDIR/thoughtd}
THOUGHTCLI=${THOUGHTCLI:-$BINDIR/thought-cli}
THOUGHTTX=${THOUGHTTX:-$BINDIR/thought-tx}
THOUGHTQT=${THOUGHTQT:-$BINDIR/qt/thought-qt}

[ ! -x $THOUGHTD ] && echo "$THOUGHTD not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
THTVER=($($THOUGHTCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for thoughtd if --version-string is not set,
# but has different outcomes for thought-qt and thought-cli.
echo "[COPYRIGHT]" > footer.h2m
$THOUGHTD --version | sed -n '1!p' >> footer.h2m

for cmd in $THOUGHTD $THOUGHTCLI $THOUGHTTX $THOUGHTQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${THTVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${THTVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
