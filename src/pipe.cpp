/*
 * SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "pipe.h"

#include <QtGlobal>
#include <algorithm>
#include <array>

namespace ValveRush
{

GameEngine::GameEngine(QObject *parent)
    : QObject(parent)
    , m_rng(QRandomGenerator::global()->generate())
{
    m_grid.resize(m_cols * m_rows);
    m_tickTimer.setInterval(33); // ~30 FPS smooth fluid animation
    connect(&m_tickTimer, &QTimer::timeout, this, &GameEngine::onTick);
}

void GameEngine::setDifficulty(Difficulty diff)
{
    if (m_difficulty == diff) {
        return;
    }
    m_difficulty = diff;
    startNewGame();
}

void GameEngine::startNewGame()
{
    m_score = 0;
    startLevel(1);
}

void GameEngine::startLevel(int level)
{
    m_level = qMax(1, level);
    m_pipesFilled = 0;
    m_fastForward = false;
    m_leakCell.reset();
    generateLevel(m_level);
    setPhase(GamePhase::PreFlow);
    m_tickTimer.start();
    Q_EMIT boardChanged();
    Q_EMIT queueChanged();
    Q_EMIT statsChanged();
}

void GameEngine::togglePause()
{
    if (m_phase == GamePhase::Paused) {
        setPhase(m_phaseBeforePause);
        m_tickTimer.start();
    } else if (m_phase == GamePhase::PreFlow || m_phase == GamePhase::Flowing) {
        m_phaseBeforePause = m_phase;
        setPhase(GamePhase::Paused);
        m_tickTimer.stop();
    }
}

void GameEngine::setFastForward(bool enabled)
{
    if (m_fastForward == enabled) {
        return;
    }
    m_fastForward = enabled;
    if (m_fastForward && m_phase == GamePhase::PreFlow) {
        m_preFlowRemaining = 0.0;
        setPhase(GamePhase::Flowing);
        Q_EMIT fluidStarted();
    }
    Q_EMIT statsChanged();
}

bool GameEngine::canPlaceAt(int col, int row) const noexcept
{
    if (col < 0 || col >= m_cols || row < 0 || row >= m_rows) {
        return false;
    }
    if (m_phase != GamePhase::PreFlow && m_phase != GamePhase::Flowing) {
        return false;
    }
    return tileAt(col, row).isPlaceable();
}

bool GameEngine::placePipeAt(int col, int row)
{
    if (!canPlaceAt(col, row) || m_queue.isEmpty()) {
        return false;
    }

    TileState &tile = m_grid[row * m_cols + col];
    const bool replaced = (tile.type != PipeType::Empty);
    if (replaced) {
        m_score = qMax(0, m_score - 50);
    }

    tile.type = m_queue.takeFirst();
    m_queue.append(drawRandomPipe());

    Q_EMIT pipePlaced(col, row, replaced);
    Q_EMIT boardChanged();
    Q_EMIT queueChanged();
    Q_EMIT statsChanged();
    return true;
}

const TileState &GameEngine::tileAt(int col, int row) const
{
    return m_grid[row * m_cols + col];
}

std::optional<Direction> GameEngine::exitDirectionFor(PipeType type, Direction entrySide, Direction specialDir) noexcept
{
    switch (type) {
    case PipeType::Empty:
    case PipeType::Blocked:
        return std::nullopt;
    case PipeType::StraightH:
        if (entrySide == Direction::West) {
            return Direction::East;
        }
        if (entrySide == Direction::East) {
            return Direction::West;
        }
        return std::nullopt;
    case PipeType::StraightV:
        if (entrySide == Direction::North) {
            return Direction::South;
        }
        if (entrySide == Direction::South) {
            return Direction::North;
        }
        return std::nullopt;
    case PipeType::CornerNE:
        if (entrySide == Direction::North) {
            return Direction::East;
        }
        if (entrySide == Direction::East) {
            return Direction::North;
        }
        return std::nullopt;
    case PipeType::CornerES:
        if (entrySide == Direction::East) {
            return Direction::South;
        }
        if (entrySide == Direction::South) {
            return Direction::East;
        }
        return std::nullopt;
    case PipeType::CornerSW:
        if (entrySide == Direction::South) {
            return Direction::West;
        }
        if (entrySide == Direction::West) {
            return Direction::South;
        }
        return std::nullopt;
    case PipeType::CornerWN:
        if (entrySide == Direction::West) {
            return Direction::North;
        }
        if (entrySide == Direction::North) {
            return Direction::West;
        }
        return std::nullopt;
    case PipeType::Cross:
        return oppositeDirection(entrySide);
    case PipeType::Start:
        return specialDir;
    case PipeType::End:
        if (entrySide == specialDir) {
            return oppositeDirection(specialDir);
        }
        return std::nullopt;
    }
    return std::nullopt;
}

void GameEngine::onTick()
{
    step(0.033);
}

void GameEngine::step(qreal dtSeconds)
{
    if (m_phase == GamePhase::PreFlow) {
        m_preFlowRemaining = qMax(0.0, m_preFlowRemaining - dtSeconds);
        if (m_preFlowRemaining <= 0.0) {
            setPhase(GamePhase::Flowing);
            Q_EMIT fluidStarted();
        }
        Q_EMIT statsChanged();
        return;
    }

    if (m_phase != GamePhase::Flowing) {
        return;
    }

    const qreal speedMultiplier = m_fastForward ? 5.5 : 1.0;
    qreal remainingProgress = m_flowSpeed * speedMultiplier * dtSeconds;

    while (remainingProgress > 0.0 && m_phase == GamePhase::Flowing) {
        TileState &current = m_grid[m_flowCell.y() * m_cols + m_flowCell.x()];
        qreal &progress = m_flowUsingSecondary ? current.secondaryProgress : current.primaryProgress;
        const qreal needed = 1.0 - progress;
        if (remainingProgress < needed) {
            progress += remainingProgress;
            remainingProgress = 0.0;
        } else {
            progress = 1.0;
            remainingProgress -= needed;
            advanceFlowCell();
        }
    }

    Q_EMIT boardChanged();
    Q_EMIT statsChanged();
}

void GameEngine::advanceFlowCell()
{
    const TileState &current = m_grid[m_flowCell.y() * m_cols + m_flowCell.x()];
    const Direction exitDir = m_flowUsingSecondary ? current.secondaryExit : current.primaryExit;

    // If we just finished filling an End valve, level is immediately won if target reached (or bonus!)
    if (current.type == PipeType::End) {
        if (m_pipesFilled >= m_targetPipes) {
            completeLevel();
        } else {
            triggerLeak(m_flowCell);
        }
        return;
    }

    const QPoint nextCell = m_flowCell + directionDelta(exitDir);
    if (nextCell.x() < 0 || nextCell.x() >= m_cols || nextCell.y() < 0 || nextCell.y() >= m_rows) {
        if (m_pipesFilled >= m_targetPipes) {
            completeLevel();
        } else {
            triggerLeak(m_flowCell);
        }
        return;
    }

    TileState &next = m_grid[nextCell.y() * m_cols + nextCell.x()];
    const Direction entrySide = oppositeDirection(exitDir);
    const auto nextExitOpt = exitDirectionFor(next.type, entrySide, next.specialDir);

    if (!nextExitOpt.has_value()) {
        if (m_pipesFilled >= m_targetPipes) {
            completeLevel();
        } else {
            triggerLeak(nextCell);
        }
        return;
    }

    // Check if the tile is already filled on that axis
    if (next.type == PipeType::Cross) {
        if (!next.hasPrimaryFlow) {
            next.hasPrimaryFlow = true;
            next.primaryEntry = entrySide;
            next.primaryExit = *nextExitOpt;
            next.primaryProgress = 0.0;
            m_flowUsingSecondary = false;
        } else if (!next.hasSecondaryFlow
                   && ((entrySide == Direction::North || entrySide == Direction::South)
                       != (next.primaryEntry == Direction::North || next.primaryEntry == Direction::South))) {
            next.hasSecondaryFlow = true;
            next.secondaryEntry = entrySide;
            next.secondaryExit = *nextExitOpt;
            next.secondaryProgress = 0.0;
            m_flowUsingSecondary = true;
            m_score += 500; // Cross-over bonus!
        } else {
            if (m_pipesFilled >= m_targetPipes) {
                completeLevel();
            } else {
                triggerLeak(nextCell);
            }
            return;
        }
    } else {
        if (next.hasPrimaryFlow) {
            if (m_pipesFilled >= m_targetPipes) {
                completeLevel();
            } else {
                triggerLeak(nextCell);
            }
            return;
        }
        next.hasPrimaryFlow = true;
        next.primaryEntry = entrySide;
        next.primaryExit = *nextExitOpt;
        next.primaryProgress = 0.0;
        m_flowUsingSecondary = false;
    }

    m_flowCell = nextCell;
    m_flowIncomingDir = entrySide;

    if (next.type != PipeType::End) {
        ++m_pipesFilled;
        int pts = 100;
        if (m_pipesFilled > m_targetPipes) {
            pts += 200; // Extra pipe bonus
        }
        if (m_fastForward) {
            pts += 50; // Fast-forward bonus
        }
        m_score += pts;
    }
}

void GameEngine::completeLevel()
{
    m_tickTimer.stop();
    const int levelBonus = m_level * 300 + qMax(0, m_pipesFilled - m_targetPipes) * 150;
    m_score += levelBonus;
    setPhase(GamePhase::LevelWon);
    Q_EMIT statsChanged();
    Q_EMIT levelCompleted(m_level, levelBonus);
}

void GameEngine::triggerLeak(const QPoint &at)
{
    m_tickTimer.stop();
    m_leakCell = at;
    setPhase(GamePhase::GameOver);
    Q_EMIT boardChanged();
    Q_EMIT statsChanged();
    Q_EMIT gameOver(m_score);
}

void GameEngine::setPhase(GamePhase p)
{
    if (m_phase == p) {
        return;
    }
    m_phase = p;
    Q_EMIT phaseChanged(m_phase);
}

void GameEngine::generateLevel(int level)
{
    std::fill(m_grid.begin(), m_grid.end(), TileState{});

    // Difficulty parameters
    qreal baseDelay = 15.0;
    qreal baseSpeed = 0.42;
    int baseTarget = 8;
    int baseObstacles = 0;

    switch (m_difficulty) {
    case Difficulty::Easy:
        baseDelay = 18.0;
        baseSpeed = 0.34;
        baseTarget = 7;
        baseObstacles = 0;
        break;
    case Difficulty::Medium:
        baseDelay = 14.0;
        baseSpeed = 0.46;
        baseTarget = 10;
        baseObstacles = 1;
        break;
    case Difficulty::Hard:
        baseDelay = 10.0;
        baseSpeed = 0.60;
        baseTarget = 12;
        baseObstacles = 3;
        break;
    }

    m_preFlowTotal = qMax(5.0, baseDelay - (level - 1) * 0.8);
    m_preFlowRemaining = m_preFlowTotal;
    m_flowSpeed = baseSpeed + (level - 1) * 0.055;
    m_targetPipes = baseTarget + (level - 1) * 2;

    // Place Start Valve away from the immediate edge so player always has room
    const int startCol = 1 + static_cast<int>(m_rng.bounded(m_cols - 3));
    const int startRow = 1 + static_cast<int>(m_rng.bounded(m_rows - 2));
    static constexpr std::array<Direction, 4> dirs = {Direction::North, Direction::East, Direction::South, Direction::West};
    const Direction startDir = dirs[m_rng.bounded(4)];

    m_startCell = QPoint(startCol, startRow);
    TileState &startTile = m_grid[startRow * m_cols + startCol];
    startTile.type = PipeType::Start;
    startTile.fixed = true;
    startTile.specialDir = startDir;
    startTile.hasPrimaryFlow = true;
    startTile.primaryEntry = oppositeDirection(startDir);
    startTile.primaryExit = startDir;
    startTile.primaryProgress = 0.0;

    m_flowCell = m_startCell;
    m_flowIncomingDir = oppositeDirection(startDir);
    m_flowUsingSecondary = false;

    // Place obstacles (never blocking the start exit cell)
    const QPoint startExitNeighbor = m_startCell + directionDelta(startDir);
    const int numObstacles = qMin(12, baseObstacles + (level - 1));
    int placed = 0;
    int attempts = 0;
    while (placed < numObstacles && attempts < 100) {
        ++attempts;
        const int c = static_cast<int>(m_rng.bounded(m_cols));
        const int r = static_cast<int>(m_rng.bounded(m_rows));
        const QPoint p(c, r);
        if (p == m_startCell || p == startExitNeighbor) {
            continue;
        }
        TileState &t = m_grid[r * m_cols + c];
        if (t.type == PipeType::Empty) {
            t.type = PipeType::Blocked;
            t.fixed = true;
            ++placed;
        }
    }

    // Populate upcoming pipe queue
    m_queue.clear();
    for (int i = 0; i < QueueSize; ++i) {
        m_queue.append(drawRandomPipe());
    }
}

PipeType GameEngine::drawRandomPipe()
{
    // Weighted distribution similar to classic Pipe Mania
    static constexpr std::array<PipeType, 14> bag = {
        PipeType::StraightH,
        PipeType::StraightH,
        PipeType::StraightV,
        PipeType::StraightV,
        PipeType::CornerNE,
        PipeType::CornerNE,
        PipeType::CornerES,
        PipeType::CornerES,
        PipeType::CornerSW,
        PipeType::CornerSW,
        PipeType::CornerWN,
        PipeType::CornerWN,
        PipeType::Cross,
        PipeType::Cross,
    };
    return bag[m_rng.bounded(static_cast<quint32>(bag.size()))];
}

void GameEngine::setTileForTest(int col, int row, PipeType type, bool fixed, Direction specialDir)
{
    TileState &t = m_grid[row * m_cols + col];
    t = TileState{};
    t.type = type;
    t.fixed = fixed;
    t.specialDir = specialDir;
}

void GameEngine::setQueueForTest(const QVector<PipeType> &queue)
{
    m_queue = queue;
}

void GameEngine::setStartCellForTest(int col, int row, Direction dir)
{
    m_startCell = QPoint(col, row);
    TileState &t = m_grid[row * m_cols + col];
    t = TileState{};
    t.type = PipeType::Start;
    t.fixed = true;
    t.specialDir = dir;
    t.hasPrimaryFlow = true;
    t.primaryEntry = oppositeDirection(dir);
    t.primaryExit = dir;
    t.primaryProgress = 0.0;
    m_flowCell = m_startCell;
    m_flowIncomingDir = oppositeDirection(dir);
    m_flowUsingSecondary = false;
    m_pipesFilled = 0;
    m_phase = GamePhase::Flowing;
}

} // namespace ValveRush
