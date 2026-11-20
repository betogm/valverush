/*
 * SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VALVERUSH_QUEUEWIDGET_H
#define VALVERUSH_QUEUEWIDGET_H

#include "pipe.h"
#include "piperenderer.h"

#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QWidget>

namespace ValveRush
{

class QueuePreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QueuePreviewWidget(GameEngine *engine, QWidget *parent = nullptr);

    void setThemePalette(const ThemePalette &palette);
    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    GameEngine *m_engine = nullptr;
    ThemePalette m_palette;
};

class SidePanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SidePanelWidget(GameEngine *engine, QWidget *parent = nullptr);

    void setThemePalette(const ThemePalette &palette);

private Q_SLOTS:
    void updateStats();
    void updatePhase(ValveRush::GamePhase phase);

private:
    GameEngine *m_engine = nullptr;
    QueuePreviewWidget *m_queuePreview = nullptr;
    QLabel *m_levelLabel = nullptr;
    QLabel *m_scoreLabel = nullptr;
    QLabel *m_pipesLabel = nullptr;
    QLabel *m_timerLabel = nullptr;
    QProgressBar *m_timerBar = nullptr;
    QPushButton *m_fastForwardBtn = nullptr;
    QPushButton *m_nextLevelBtn = nullptr;
};

} // namespace ValveRush

#endif // VALVERUSH_QUEUEWIDGET_H
