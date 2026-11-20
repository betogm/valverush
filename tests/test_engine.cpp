/*
 * SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "pipe.h"

#include <QTest>

using namespace ValveRush;

class TestEngine : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testExitDirections()
    {
        QCOMPARE(GameEngine::exitDirectionFor(PipeType::StraightH, Direction::West), Direction::East);
        QCOMPARE(GameEngine::exitDirectionFor(PipeType::StraightH, Direction::East), Direction::West);
        QVERIFY(!GameEngine::exitDirectionFor(PipeType::StraightH, Direction::North).has_value());

        QCOMPARE(GameEngine::exitDirectionFor(PipeType::CornerES, Direction::East), Direction::South);
        QCOMPARE(GameEngine::exitDirectionFor(PipeType::CornerES, Direction::South), Direction::East);
        QVERIFY(!GameEngine::exitDirectionFor(PipeType::CornerES, Direction::North).has_value());

        QCOMPARE(GameEngine::exitDirectionFor(PipeType::Cross, Direction::North), Direction::South);
        QCOMPARE(GameEngine::exitDirectionFor(PipeType::Cross, Direction::West), Direction::East);
    }

    void testPlaceAndReplacePipe()
    {
        GameEngine engine;
        engine.startNewGame();
        engine.setQueueForTest({PipeType::StraightH, PipeType::CornerES, PipeType::Cross, PipeType::StraightV, PipeType::CornerNE});

        // Find an empty cell
        int targetCol = -1;
        int targetRow = -1;
        for (int r = 0; r < engine.rows() && targetCol < 0; ++r) {
            for (int c = 0; c < engine.cols(); ++c) {
                if (engine.tileAt(c, r).type == PipeType::Empty) {
                    targetCol = c;
                    targetRow = r;
                    break;
                }
            }
        }
        QVERIFY(targetCol >= 0);

        QVERIFY(engine.placePipeAt(targetCol, targetRow));
        QCOMPARE(engine.tileAt(targetCol, targetRow).type, PipeType::StraightH);

        // Replace existing unfilled pipe
        QVERIFY(engine.placePipeAt(targetCol, targetRow));
        QCOMPARE(engine.tileAt(targetCol, targetRow).type, PipeType::CornerES);
    }

    void testFlowAndLeak()
    {
        GameEngine engine;
        engine.startNewGame();
        for (int r = 0; r < engine.rows(); ++r) {
            for (int c = 0; c < engine.cols(); ++c) {
                engine.setTileForTest(c, r, PipeType::Empty, false);
            }
        }
        engine.setStartCellForTest(1, 3, Direction::East);
        engine.setTileForTest(2, 3, PipeType::StraightH, false);
        engine.setTileForTest(3, 3, PipeType::CornerSW, false);
        engine.setTileForTest(3, 4, PipeType::StraightV, false);

        // Advance simulation enough to traverse Start -> (2,3) -> (3,3) -> (3,4) -> leak at (3,5)
        for (int i = 0; i < 60; ++i) {
            engine.step(0.25);
        }

        QCOMPARE(engine.pipesFilled(), 3);
        QCOMPARE(engine.phase(), GamePhase::GameOver);
        QVERIFY(engine.leakCell().has_value());
        QCOMPARE(*engine.leakCell(), QPoint(3, 5));
    }

    void testCrossDoubleTraversalBonus()
    {
        GameEngine engine;
        engine.startNewGame();
        for (int r = 0; r < engine.rows(); ++r) {
            for (int c = 0; c < engine.cols(); ++c) {
                engine.setTileForTest(c, r, PipeType::Empty, false);
            }
        }
        // Loop back through Cross at (3,3):
        // (1,3) Start->East -> (2,3) StraightH -> (3,3) Cross (W->E) -> (4,3) CornerWN (W->N)
        // -> (4,2) CornerSW (S->W) -> (3,2) CornerES (E->S) -> (3,3) Cross (N->S)!
        engine.setStartCellForTest(1, 3, Direction::East);
        engine.setTileForTest(2, 3, PipeType::StraightH, false);
        engine.setTileForTest(3, 3, PipeType::Cross, false);
        engine.setTileForTest(4, 3, PipeType::CornerWN, false);
        engine.setTileForTest(4, 2, PipeType::CornerSW, false);
        engine.setTileForTest(3, 2, PipeType::CornerES, false);

        for (int i = 0; i < 80; ++i) {
            engine.step(0.25);
        }

        const TileState &crossTile = engine.tileAt(3, 3);
        QVERIFY(crossTile.hasPrimaryFlow);
        QVERIFY(crossTile.hasSecondaryFlow);
        QVERIFY(engine.score() >= 500);
    }
};

QTEST_MAIN(TestEngine)
#include "test_engine.moc"
