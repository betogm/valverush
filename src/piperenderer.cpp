/*
 * SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "piperenderer.h"

#include <QPainterPath>

namespace ValveRush
{

ThemePalette ThemePalette::fromThemeId(const QString &themeId)
{
    if (themeId.contains(QStringLiteral("neon"), Qt::CaseInsensitive)) {
        ThemePalette p;
        p.id = QStringLiteral("neon");
        p.boardBg = QColor(14, 12, 29);
        p.cellBg = QColor(24, 21, 48);
        p.cellAltBg = QColor(28, 25, 56);
        p.cellBorder = QColor(67, 56, 130);
        p.hoverHighlight = QColor(236, 72, 153, 95);
        p.pipeOuter = QColor(109, 40, 217);
        p.pipeInner = QColor(30, 27, 75);
        p.pipeFlange = QColor(192, 132, 252);
        p.fluidColor = QColor(34, 211, 238);
        p.fluidHighlight = QColor(207, 250, 254);
        p.valveColor = QColor(244, 114, 182);
        p.obstacleColor = QColor(55, 48, 107);
        p.leakColor = QColor(244, 63, 94);
        return p;
    }

    if (themeId.contains(QStringLiteral("symbian"), Qt::CaseInsensitive)) {
        ThemePalette p;
        p.id = QStringLiteral("symbian");
        p.boardBg = QColor(22, 38, 40);
        p.cellBg = QColor(36, 58, 61);
        p.cellAltBg = QColor(41, 64, 67);
        p.cellBorder = QColor(74, 108, 112);
        p.hoverHighlight = QColor(163, 230, 53, 85);
        p.pipeOuter = QColor(148, 163, 184);
        p.pipeInner = QColor(51, 65, 85);
        p.pipeFlange = QColor(203, 213, 225);
        p.fluidColor = QColor(132, 204, 22);
        p.fluidHighlight = QColor(217, 249, 157);
        p.valveColor = QColor(250, 204, 21);
        p.obstacleColor = QColor(71, 85, 105);
        p.leakColor = QColor(239, 68, 68);
        return p;
    }

    return ThemePalette{};
}

QPointF PipeRenderer::edgeCenter(const QRectF &rect, Direction dir)
{
    switch (dir) {
    case Direction::North:
        return QPointF(rect.center().x(), rect.top());
    case Direction::East:
        return QPointF(rect.right(), rect.center().y());
    case Direction::South:
        return QPointF(rect.center().x(), rect.bottom());
    case Direction::West:
        return QPointF(rect.left(), rect.center().y());
    }
    return rect.center();
}

void PipeRenderer::drawSegment(QPainter &painter, const QRectF &rect, Direction d1, Direction d2, qreal pipeWidth, const QColor &color)
{
    const QPointF p1 = edgeCenter(rect, d1);
    const QPointF p2 = edgeCenter(rect, d2);
    const QPointF c = rect.center();

    QPainterPath path;
    path.moveTo(p1);
    if (d1 == oppositeDirection(d2)) {
        path.lineTo(p2);
    } else {
        path.quadTo(c, p2);
    }

    QPen pen(color, pipeWidth, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

void PipeRenderer::drawPartialFlow(QPainter &painter,
                                   const QRectF &rect,
                                   Direction entry,
                                   Direction exit,
                                   qreal progress,
                                   qreal fluidWidth,
                                   const ThemePalette &palette)
{
    if (progress <= 0.0) {
        return;
    }
    const qreal clamped = qBound(0.0, progress, 1.0);
    const QPointF p1 = edgeCenter(rect, entry);
    const QPointF p2 = edgeCenter(rect, exit);
    const QPointF c = rect.center();

    QPainterPath fullPath;
    fullPath.moveTo(p1);
    if (entry == oppositeDirection(exit)) {
        fullPath.lineTo(p2);
    } else {
        fullPath.quadTo(c, p2);
    }

    QPainterPath subPath;
    constexpr int steps = 16;
    const int maxStep = qMax(1, static_cast<int>(steps * clamped));
    subPath.moveTo(fullPath.pointAtPercent(0.0));
    for (int i = 1; i <= maxStep; ++i) {
        const qreal t = (static_cast<qreal>(i) / steps);
        if (t <= clamped) {
            subPath.lineTo(fullPath.pointAtPercent(t));
        }
    }
    subPath.lineTo(fullPath.pointAtPercent(clamped));

    QPen outerFluid(palette.fluidColor, fluidWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(outerFluid);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(subPath);

    QPen coreFluid(palette.fluidHighlight, fluidWidth * 0.35, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(coreFluid);
    painter.drawPath(subPath);
}

void PipeRenderer::drawFlange(QPainter &painter, const QRectF &rect, Direction dir, qreal pipeWidth, const QColor &color)
{
    const qreal flangeLen = pipeWidth * 1.28;
    const qreal flangeThick = qMax(2.5, pipeWidth * 0.22);
    const QPointF p = edgeCenter(rect, dir);

    QRectF fRect;
    if (dir == Direction::North || dir == Direction::South) {
        const qreal y = (dir == Direction::North) ? p.y() : (p.y() - flangeThick);
        fRect = QRectF(p.x() - flangeLen / 2.0, y, flangeLen, flangeThick);
    } else {
        const qreal x = (dir == Direction::West) ? p.x() : (p.x() - flangeLen);
        fRect = QRectF(x, p.y() - flangeLen / 2.0, flangeThick, flangeLen);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(fRect, 1.5, 1.5);
}

void PipeRenderer::drawPipePreview(QPainter &painter, const QRectF &rect, PipeType type, const ThemePalette &palette)
{
    TileState dummy;
    dummy.type = type;
    drawTile(painter, rect, dummy, palette);
}

void PipeRenderer::drawTile(QPainter &painter,
                            const QRectF &rect,
                            const TileState &tile,
                            const ThemePalette &palette,
                            bool isHovered,
                            bool isCursor,
                            bool isLeak,
                            qreal preFlowRatio)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal pipeOuterW = rect.width() * 0.38;
    const qreal pipeInnerW = rect.width() * 0.24;
    const qreal fluidW = rect.width() * 0.18;
    const bool narrow = GameEngine::isNarrowPipe(tile.type);
    const qreal activeFluidW = narrow ? rect.width() * 0.08 : fluidW;

    auto drawPipeShape = [&](Direction d1, Direction d2) {
        drawSegment(painter, rect, d1, d2, pipeOuterW, palette.pipeOuter);
        drawSegment(painter, rect, d1, d2, pipeInnerW, narrow ? palette.pipeOuter.darker(135) : palette.pipeInner);
        if (narrow) {
            drawSegment(painter, rect, d1, d2, rect.width() * 0.11, palette.pipeInner);
            // Subtle reducer collars near each end to highlight the flow-slowing constriction
            const qreal collarThick = qMax(2.0, pipeOuterW * 0.16);
            const qreal collarLen = pipeOuterW * 0.92;
            const QPointF c = rect.center();
            const qreal offset = rect.width() * 0.24;
            painter.setPen(Qt::NoPen);
            painter.setBrush(palette.valveColor);
            if (d1 == Direction::West || d1 == Direction::East) {
                painter.drawRoundedRect(QRectF(c.x() - offset - collarThick / 2.0, c.y() - collarLen / 2.0, collarThick, collarLen), 1.0, 1.0);
                painter.drawRoundedRect(QRectF(c.x() + offset - collarThick / 2.0, c.y() - collarLen / 2.0, collarThick, collarLen), 1.0, 1.0);
            } else {
                painter.drawRoundedRect(QRectF(c.x() - collarLen / 2.0, c.y() - offset - collarThick / 2.0, collarLen, collarThick), 1.0, 1.0);
                painter.drawRoundedRect(QRectF(c.x() - collarLen / 2.0, c.y() + offset - collarThick / 2.0, collarLen, collarThick), 1.0, 1.0);
            }
        }
        drawFlange(painter, rect, d1, pipeOuterW, palette.pipeFlange);
        drawFlange(painter, rect, d2, pipeOuterW, palette.pipeFlange);
    };

    switch (tile.type) {
    case PipeType::Empty:
        break;
    case PipeType::StraightH:
    case PipeType::NarrowH:
        drawPipeShape(Direction::West, Direction::East);
        break;
    case PipeType::StraightV:
    case PipeType::NarrowV:
        drawPipeShape(Direction::North, Direction::South);
        break;
    case PipeType::CornerNE:
        drawPipeShape(Direction::North, Direction::East);
        break;
    case PipeType::CornerES:
        drawPipeShape(Direction::East, Direction::South);
        break;
    case PipeType::CornerSW:
        drawPipeShape(Direction::South, Direction::West);
        break;
    case PipeType::CornerWN:
        drawPipeShape(Direction::West, Direction::North);
        break;
    case PipeType::Cross:
        // Horizontal layer underneath, vertical bridge on top
        drawPipeShape(Direction::West, Direction::East);
        if (tile.hasPrimaryFlow && (tile.primaryEntry == Direction::West || tile.primaryEntry == Direction::East)) {
            drawPartialFlow(painter, rect, tile.primaryEntry, tile.primaryExit, tile.primaryProgress, fluidW, palette);
        } else if (tile.hasSecondaryFlow && (tile.secondaryEntry == Direction::West || tile.secondaryEntry == Direction::East)) {
            drawPartialFlow(painter, rect, tile.secondaryEntry, tile.secondaryExit, tile.secondaryProgress, fluidW, palette);
        }
        drawPipeShape(Direction::North, Direction::South);
        if (tile.hasPrimaryFlow && (tile.primaryEntry == Direction::North || tile.primaryEntry == Direction::South)) {
            drawPartialFlow(painter, rect, tile.primaryEntry, tile.primaryExit, tile.primaryProgress, fluidW, palette);
        } else if (tile.hasSecondaryFlow && (tile.secondaryEntry == Direction::North || tile.secondaryEntry == Direction::South)) {
            drawPartialFlow(painter, rect, tile.secondaryEntry, tile.secondaryExit, tile.secondaryProgress, fluidW, palette);
        }
        break;
    case PipeType::Start: {
        // Draw outlet spout
        const QPointF c = rect.center();
        const QPointF exitPt = edgeCenter(rect, tile.specialDir);
        painter.setPen(QPen(palette.pipeOuter, pipeOuterW, Qt::SolidLine, Qt::FlatCap));
        painter.drawLine(c, exitPt);
        painter.setPen(QPen(palette.pipeInner, pipeInnerW, Qt::SolidLine, Qt::FlatCap));
        painter.drawLine(c, exitPt);
        drawFlange(painter, rect, tile.specialDir, pipeOuterW, palette.pipeFlange);

        // Animated fluid inside spout if flowing
        if (tile.hasPrimaryFlow && tile.primaryProgress > 0.0) {
            const QPointF fluidEnd = c + (exitPt - c) * qBound(0.0, tile.primaryProgress, 1.0);
            painter.setPen(QPen(palette.fluidColor, fluidW, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(c, fluidEnd);
        }

        // Valve wheel body
        const qreal r = rect.width() * 0.28;
        painter.setPen(QPen(palette.valveColor.darker(140), 2.5));
        painter.setBrush(palette.valveColor);
        painter.drawEllipse(c, r, r);

        // Pre-flow countdown ring
        if (preFlowRatio > 0.0 && preFlowRatio < 1.0) {
            painter.setPen(QPen(palette.fluidColor, 3.0, Qt::SolidLine, Qt::RoundCap));
            painter.setBrush(Qt::NoBrush);
            const QRectF arcRect(c.x() - r * 1.2, c.y() - r * 1.2, r * 2.4, r * 2.4);
            painter.drawArc(arcRect, 90 * 16, static_cast<int>(-360 * 16 * preFlowRatio));
        }

        // Valve spokes
        painter.setPen(QPen(palette.boardBg, 2.2, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(c.x() - r * 0.55, c.y()), QPointF(c.x() + r * 0.55, c.y()));
        painter.drawLine(QPointF(c.x(), c.y() - r * 0.55), QPointF(c.x(), c.y() + r * 0.55));
        break;
    }
    case PipeType::End: {
        const QPointF c = rect.center();
        const QPointF entryPt = edgeCenter(rect, tile.specialDir);
        painter.setPen(QPen(palette.pipeOuter, pipeOuterW, Qt::SolidLine, Qt::FlatCap));
        painter.drawLine(entryPt, c);
        painter.setPen(QPen(palette.pipeInner, pipeInnerW, Qt::SolidLine, Qt::FlatCap));
        painter.drawLine(entryPt, c);
        drawFlange(painter, rect, tile.specialDir, pipeOuterW, palette.pipeFlange);
        painter.setPen(QPen(palette.fluidColor, 2.0));
        painter.setBrush(palette.boardBg);
        painter.drawEllipse(c, rect.width() * 0.22, rect.width() * 0.22);
        break;
    }
    case PipeType::Blocked: {
        const QRectF obs = rect.adjusted(rect.width() * 0.14, rect.height() * 0.14, -rect.width() * 0.14, -rect.height() * 0.14);
        painter.setPen(QPen(palette.obstacleColor.lighter(130), 1.5));
        painter.setBrush(palette.obstacleColor);
        painter.drawRoundedRect(obs, 6.0, 6.0);
        painter.setPen(QPen(palette.boardBg, 2.0));
        painter.drawLine(obs.topLeft() + QPointF(6, 6), obs.bottomRight() - QPointF(6, 6));
        painter.drawLine(obs.topRight() + QPointF(-6, 6), obs.bottomLeft() + QPointF(6, -6));
        break;
    }
    }

    // Primary flow for non-Cross, non-Start pipes
    if (tile.type != PipeType::Cross && tile.type != PipeType::Start && tile.hasPrimaryFlow) {
        drawPartialFlow(painter, rect, tile.primaryEntry, tile.primaryExit, tile.primaryProgress, activeFluidW, palette);
    }

    if (isLeak) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(palette.leakColor.red(), palette.leakColor.green(), palette.leakColor.blue(), 150));
        painter.drawEllipse(rect.center(), rect.width() * 0.34, rect.height() * 0.34);
    }

    if (isHovered) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(palette.hoverHighlight);
        painter.drawRoundedRect(rect.adjusted(2, 2, -2, -2), 4.0, 4.0);
    }

    if (isCursor) {
        painter.setPen(QPen(palette.fluidColor, 2.2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect.adjusted(2, 2, -2, -2), 4.0, 4.0);
    }

    painter.restore();
}

} // namespace ValveRush
