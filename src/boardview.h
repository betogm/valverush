/*
 * SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VALVERUSH_BOARDVIEW_H
#define VALVERUSH_BOARDVIEW_H

#include "pipe.h"
#include "piperenderer.h"

#include <QPoint>
#include <QWidget>

namespace ValveRush
{

class BoardView : public QWidget
{
    Q_OBJECT

public:
    explicit BoardView(GameEngine *engine, QWidget *parent = nullptr);

    void setThemePalette(const ThemePalette &palette);
    void setShowCoordinates(bool show);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    [[nodiscard]] QRectF gridRect() const;
    [[nodiscard]] QPoint cellFromPos(const QPointF &pos) const;

    GameEngine *m_engine = nullptr;
    ThemePalette m_palette;
    bool m_showCoordinates = false;
    QPoint m_hoverCell{-1, -1};
    QPoint m_cursorCell{0, 0};
};

} // namespace ValveRush

#endif // VALVERUSH_BOARDVIEW_H
