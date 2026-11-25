/*
 * SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VALVERUSH_PIPE_H
#define VALVERUSH_PIPE_H

#include <QObject>
#include <QPoint>
#include <QRandomGenerator>
#include <QTimer>
#include <QVector>

#include <optional>
#include <utility>

namespace ValveRush
{

enum class Direction {
    North = 0,
    East = 1,
    South = 2,
    West = 3,
};

[[nodiscard]] constexpr Direction oppositeDirection(Direction dir) noexcept
{
    switch (dir) {
    case Direction::North:
        return Direction::South;
    case Direction::East:
        return Direction::West;
    case Direction::South:
        return Direction::North;
    case Direction::West:
        return Direction::East;
    }
    return Direction::North;
}

[[nodiscard]] constexpr QPoint directionDelta(Direction dir) noexcept
{
    switch (dir) {
    case Direction::North:
        return QPoint(0, -1);
    case Direction::East:
        return QPoint(1, 0);
    case Direction::South:
        return QPoint(0, 1);
    case Direction::West:
        return QPoint(-1, 0);
    }
    return QPoint(0, 0);
}

enum class PipeType {
    Empty = 0,
    StraightH, // West <-> East
    StraightV, // North <-> South
    NarrowH, // West <-> East (thinner, slows fluid flow)
    NarrowV, // North <-> South (thinner, slows fluid flow)
    CornerNE, // North <-> East
    CornerES, // East <-> South
    CornerSW, // South <-> West
    CornerWN, // West <-> North
    Cross, // North <-> South AND West <-> East
    Start, // Source valve (1-way exit)
    End, // Target drain valve (1-way entry)
    Blocked, // Solid obstacle
};

enum class Difficulty {
    Easy = 0,
    Medium = 1,
    Hard = 2,
};

enum class GamePhase {
    Ready, // Not yet started
    PreFlow, // Countdown before valve opens
    Flowing, // Fluid is moving through pipes
    Paused, // Paused by player
    LevelWon, // Target pipe count reached (and/or End valve reached)
    GameOver, // Fluid leaked before reaching target
};

struct TileState {
    PipeType type = PipeType::Empty;
    bool fixed = false;

    // Primary flow (or horizontal/vertical first pass for Cross)
    bool hasPrimaryFlow = false;
    Direction primaryEntry = Direction::West;
    Direction primaryExit = Direction::East;
    qreal primaryProgress = 0.0; // 0.0 .. 1.0

    // Secondary flow (only used for second perpendicular pass through PipeType::Cross)
    bool hasSecondaryFlow = false;
    Direction secondaryEntry = Direction::North;
    Direction secondaryExit = Direction::South;
    qreal secondaryProgress = 0.0; // 0.0 .. 1.0

    // For Start and End tiles
    Direction specialDir = Direction::East;

    [[nodiscard]] bool isFilled() const noexcept
    {
        return hasPrimaryFlow || hasSecondaryFlow;
    }

    [[nodiscard]] bool isPlaceable() const noexcept
    {
        return !fixed && !isFilled();
    }
};

/**
 * Pure game state & simulation engine for ValveRush (Pipe Mania).
 */
class GameEngine : public QObject
{
    Q_OBJECT

public:
    static constexpr int DefaultCols = 10;
    static constexpr int DefaultRows = 7;
    static constexpr int QueueSize = 5;

    explicit GameEngine(QObject *parent = nullptr);

    void setDifficulty(Difficulty diff);
    [[nodiscard]] Difficulty difficulty() const noexcept
    {
        return m_difficulty;
    }

    void startNewGame();
    void startLevel(int level);
    void togglePause();
    void setFastForward(bool enabled);
    [[nodiscard]] bool isFastForward() const noexcept
    {
        return m_fastForward;
    }

    bool placePipeAt(int col, int row);
    [[nodiscard]] bool canPlaceAt(int col, int row) const noexcept;

    [[nodiscard]] int cols() const noexcept
    {
        return m_cols;
    }
    [[nodiscard]] int rows() const noexcept
    {
        return m_rows;
    }
    [[nodiscard]] int level() const noexcept
    {
        return m_level;
    }
    [[nodiscard]] int score() const noexcept
    {
        return m_score;
    }
    [[nodiscard]] int pipesFilled() const noexcept
    {
        return m_pipesFilled;
    }
    [[nodiscard]] int targetPipes() const noexcept
    {
        return m_targetPipes;
    }
    [[nodiscard]] qreal preFlowRemaining() const noexcept
    {
        return m_preFlowRemaining;
    }
    [[nodiscard]] qreal preFlowTotal() const noexcept
    {
        return m_preFlowTotal;
    }
    [[nodiscard]] GamePhase phase() const noexcept
    {
        return m_phase;
    }

    [[nodiscard]] const TileState &tileAt(int col, int row) const;
    [[nodiscard]] const QVector<PipeType> &queue() const noexcept
    {
        return m_queue;
    }
    [[nodiscard]] QPoint activeFlowCell() const noexcept
    {
        return m_flowCell;
    }
    [[nodiscard]] std::optional<QPoint> leakCell() const noexcept
    {
        return m_leakCell;
    }

    /**
     * Returns the exit direction if a pipe of @p type (with optional @p specialDir for Start/End)
     * accepts fluid entering from @p entrySide.
     */
    [[nodiscard]] static std::optional<Direction> exitDirectionFor(PipeType type, Direction entrySide, Direction specialDir = Direction::East) noexcept;

    [[nodiscard]] static constexpr bool isNarrowPipe(PipeType type) noexcept
    {
        return type == PipeType::NarrowH || type == PipeType::NarrowV;
    }

    /**
     * Advances simulation by @p dtSeconds (exposed for deterministic unit tests as well as timer).
     */
    void step(qreal dtSeconds);

    // Test / deterministic helpers
    void setTileForTest(int col, int row, PipeType type, bool fixed = false, Direction specialDir = Direction::East);
    void setQueueForTest(const QVector<PipeType> &queue);
    void setStartCellForTest(int col, int row, Direction dir);

Q_SIGNALS:
    void boardChanged();
    void queueChanged();
    void statsChanged();
    void phaseChanged(ValveRush::GamePhase newPhase);
    void pipePlaced(int col, int row, bool replaced);
    void fluidStarted();
    void levelCompleted(int level, int bonusPoints);
    void gameOver(int finalScore);

private Q_SLOTS:
    void onTick();

private:
    void generateLevel(int level);
    PipeType drawRandomPipe();
    void advanceFlowCell();
    void completeLevel();
    void triggerLeak(const QPoint &at);
    void setPhase(GamePhase p);

    int m_cols = DefaultCols;
    int m_rows = DefaultRows;
    QVector<TileState> m_grid;
    QVector<PipeType> m_queue;

    Difficulty m_difficulty = Difficulty::Medium;
    GamePhase m_phase = GamePhase::Ready;
    GamePhase m_phaseBeforePause = GamePhase::PreFlow;

    int m_level = 1;
    int m_score = 0;
    int m_pipesFilled = 0;
    int m_targetPipes = 10;

    qreal m_preFlowTotal = 14.0;
    qreal m_preFlowRemaining = 14.0;
    qreal m_flowSpeed = 0.45; // tiles per second
    bool m_fastForward = false;

    QPoint m_startCell{1, 3};
    QPoint m_flowCell{1, 3};
    Direction m_flowIncomingDir = Direction::West; // side from which fluid enters m_flowCell
    bool m_flowUsingSecondary = false;
    std::optional<QPoint> m_leakCell;

    QTimer m_tickTimer;
    QRandomGenerator m_rng;
};

} // namespace ValveRush

#endif // VALVERUSH_PIPE_H
