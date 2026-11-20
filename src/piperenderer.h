/*
 * SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VALVERUSH_PIPERENDERER_H
#define VALVERUSH_PIPERENDERER_H

#include "pipe.h"

#include <QColor>
#include <QPainter>
#include <QRectF>

namespace ValveRush
{

struct ThemePalette {
    QString id = QStringLiteral("breeze");
    QColor boardBg{30, 34, 42};
    QColor cellBg{40, 45, 55};
    QColor cellAltBg{44, 50, 60};
    QColor cellBorder{62, 70, 84};
    QColor hoverHighlight{61, 174, 233, 90};
    QColor pipeOuter{95, 108, 128};
    QColor pipeInner{52, 60, 74};
    QColor pipeFlange{148, 163, 184};
    QColor fluidColor{29, 209, 224};
    QColor fluidHighlight{165, 243, 252};
    QColor valveColor{245, 158, 11};
    QColor obstacleColor{75, 85, 99};
    QColor leakColor{239, 68, 68};

    [[nodiscard]] static ThemePalette fromThemeId(const QString &themeId);
};

/**
 * Shared vector renderer for pipe tiles, valves, obstacles, and animated fluid flow.
 */
class PipeRenderer
{
public:
    static void drawTile(QPainter &painter,
                         const QRectF &rect,
                         const TileState &tile,
                         const ThemePalette &palette,
                         bool isHovered = false,
                         bool isCursor = false,
                         bool isLeak = false,
                         qreal preFlowRatio = 1.0);

    static void drawPipePreview(QPainter &painter, const QRectF &rect, PipeType type, const ThemePalette &palette);

private:
    static void drawSegment(QPainter &painter, const QRectF &rect, Direction d1, Direction d2, qreal pipeWidth, const QColor &color);
    static void
    drawPartialFlow(QPainter &painter, const QRectF &rect, Direction entry, Direction exit, qreal progress, qreal fluidWidth, const ThemePalette &palette);
    static void drawFlange(QPainter &painter, const QRectF &rect, Direction dir, qreal pipeWidth, const QColor &color);
    static QPointF edgeCenter(const QRectF &rect, Direction dir);
};

} // namespace ValveRush

#endif // VALVERUSH_PIPERENDERER_H
