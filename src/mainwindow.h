/*
 * SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VALVERUSH_MAINWINDOW_H
#define VALVERUSH_MAINWINDOW_H

#include "boardview.h"
#include "pipe.h"
#include "queuewidget.h"

#include <KGameDifficulty>
#include <KGameThemeProvider>
#include <KXmlGuiWindow>
#include <QAction>

namespace ValveRush
{

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private Q_SLOTS:
    void onNewGame();
    void onPauseGame(bool paused);
    void onShowHighScores();
    void onConfigureTheme();
    void onDifficultyChanged(const KGameDifficultyLevel *level);
    void onThemeChanged(const KGameTheme *theme);
    void onLevelCompleted(int level, int bonusPoints);
    void onGameOver(int finalScore);
    void onToggleCoordinates(bool show);

private:
    void setupActions();
    void setupDifficulty();

    GameEngine *m_engine = nullptr;
    BoardView *m_boardView = nullptr;
    SidePanelWidget *m_sidePanel = nullptr;
    KGameDifficulty *m_difficulty = nullptr;
    KGameThemeProvider *m_themeProvider = nullptr;
    QAction *m_pauseAction = nullptr;
    QAction *m_fastForwardAction = nullptr;
};

} // namespace ValveRush

#endif // VALVERUSH_MAINWINDOW_H
