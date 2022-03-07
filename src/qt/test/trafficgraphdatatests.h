// Copyright (c) 2018-2022 Thought Network Ltd
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef THOUGHT_QT_TEST_TRAFFICGRAPHDATATESTS_H
#define THOUGHT_QT_TEST_TRAFFICGRAPHDATATESTS_H

#include <QObject>
#include <QTest>

class TrafficGraphDataTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void simpleCurrentSampleQueueTests();
    void accumulationCurrentSampleQueueTests();
    void getRangeTests();
    void switchRangeTests();
    void clearTests();
    void averageBandwidthTest();
    void averageBandwidthEvery2EmptyTest();

};


#endif // THOUGHT_QT_TEST_TRAFFICGRAPHDATATESTS_H
