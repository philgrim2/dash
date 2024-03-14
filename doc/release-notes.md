Thought Core version 0.19.0
==========================

Release is now available from:

  <https://github.com/thoughtnetwork/thought-wallet>

This is a new hotfix release.


Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Thought-Qt (on Mac) or
thoughtcore/bin/thought-qt (on Linux). If you upgrade after DIP0003 activation and you were
using version < 0.13 you will have to reindex (start with -reindex-chainstate
or -reindex) to make sure your wallet has all the new data synced. Upgrading
from version 0.13 should not require any additional actions.

Downgrade warning
-----------------

### Downgrade to a version < 0.19.0

Downgrading to a version older than 0.19.0 is no longer supported due to
changes in the "evodb" database format. If you need to use an older version,
you must either reindex or re-sync the whole chain.

