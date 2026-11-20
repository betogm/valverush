/*
 * SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "boardview.h"

#include <KLocalizedString>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

namespace ValveRush
{

BoardView::BoardView(GameEngine *engine, QWidget *parent)
    : QWidget(parent)
    , m_engine(engine)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName(i18n("Pipe Board"));
    setAccessibleDescription(i18n("Interactive pipe grid. Click or press Enter to place the next pipe."));

    connect(m_engine, &GameEngine::boardChanged, this, qOverload<>(&QWidget::update));
    connect(m_engine, &GameEngine::phaseChanged, this, qOverload<>(&QWidget::update));
}

void BoardView::setThemePalette(const ThemePalette &palette)
{
    m_palette = palette;
    update();
}

void BoardView::setShowCoordinates(bool show)
{
    m_showCoordinates = show;
    update();
}

QSize BoardView::sizeHint() const
{
    return QSize(680, 480);
}

QSize BoardView::minimumSizeHint() const
{
    return QSize(420, 300);
}

QRectF BoardView::gridRect() const
{
    const int cols = m_engine->cols();
    const int rows = m_engine->rows();
    const qreal availW = width() - 16.0;
    const qreal availH = height() - 16.0;
    const qreal cellSize = qFloor(qMin(availW / cols, availH / rows));
    const qreal totalW = cellSize * cols;
    const qreal totalH = cellSize * rows;
    const qreal left = (width() - totalW) / 2.0;
    const qreal top = (height() - totalH) / 2.0;
    return QRectF(left, top, totalW, totalH);
}

QPoint BoardView::cellFromPos(const QPointF &pos) const
{
    const QRectF gr = gridRect();
    if (!gr.contains(pos)) {
        return QPoint(-1, -1);
    }
    const qreal cellSize = gr.width() / m_engine->cols();
    const int col = static_cast<int>((pos.x() - gr.left()) / cellSize);
    const int row = static_cast<int>((pos.y() - gr.top()) / cellSize);
    if (col < 0 || col >= m_engine->cols() || row < 0 || row >= m_engine->rows()) {
        return QPoint(-1, -1);
    }
    return QPoint(col, row);
}

void BoardView::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), m_palette.boardBg);

    const QRectF gr = gridRect();
    const int cols = m_engine->cols();
    const int rows = m_engine->rows();
    const qreal cellSize = gr.width() / cols;

    qreal preFlowRatio = 0.0;
    if (m_engine->phase() == GamePhase::PreFlow && m_engine->preFlowTotal() > 0.0) {
        preFlowRatio = m_engine->preFlowRemaining() / m_engine->preFlowTotal();
    }

    const auto leak = m_engine->leakCell();

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const QRectF cellRect(gr.left() + c * cellSize, gr.top() + r * cellSize, cellSize, cellSize);
            const bool alt = ((c + r) % 2) == 1;
            painter.fillRect(cellRect, alt ? m_palette.cellAltBg : m_palette.cellBg);
            painter.setPen(QPen(m_palette.cellBorder, 1.0));
            painter.drawRect(cellRect);

            if (m_showCoordinates) {
                painter.setPen(QColor(255, 255, 255, 45));
                QFont f = painter.font();
                f.setPointSize(7);
                painter.setFont(f);
                painter.drawText(cellRect.adjusted(3, 2, -2, -2), Qt::AlignTop | Qt::AlignLeft, QStringLiteral("%1,%2").arg(c + 1).arg(r + 1));
            }

            const QPoint pt(c, r);
            const bool hovered = (m_hoverCell == pt) && m_engine->canPlaceAt(c, r);
            const bool cursor = hasFocus() && (m_cursorCell == pt);
            const bool isLeak = leak.has_value() && (*leak == pt);

            PipeRenderer::drawTile(painter, cellRect, m_engine->tileAt(c, r), m_palette, hovered, cursor, isLeak, preFlowRatio);
        }
    }

    // Outer frame
    painter.setPen(QPen(m_palette.pipeFlange, 2.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(gr);

    // Overlay banner for Paused / LevelWon / GameOver
    if (m_engine->phase() == GamePhase::Paused || m_engine->phase() == GamePhase::LevelWon || m_engine->phase() == GamePhase::GameOver) {
        painter.fillRect(gr, QColor(15, 23, 42, 175));
        QFont f = painter.font();
        f.setPointSize(20);
        f.setBold(true);
        painter.setFont(f);
        painter.setPen(Qt::white);

        QString text;
        if (m_engine->phase() == GamePhase::Paused) {
            text = i18n("PAUSED");
        } else if (m_engine->phase() == GamePhase::LevelWon) {
            text = i18n("LEVEL %1 COMPLETE!", m_engine->level());
        } else {
            text = i18n("PIPE LEAKED — GAME OVER");
        }
        painter.drawText(gr, Qt::AlignCenter, text);
    }
}

void BoardView::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint cell = cellFromPos(event->position());
    if (cell != m_hoverCell) {
        m_hoverCell = cell;
        update();
    }
}

void BoardView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    setFocus();
    const QPoint cell = cellFromPos(event->position());
    if (cell.x() >= 0 && cell.y() >= 0) {
        m_cursorCell = cell;
        m_engine->placePipeAt(cell.x(), cell.y());
    }
}

void BoardView::leaveEvent(QEvent * /*event*/)
{
    if (m_hoverCell != QPoint(-1, -1)) {
        m_hoverCell = QPoint(-1, -1);
        update();
    }
}

void BoardView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        m_cursorCell.setX((m_cursorCell.x() - 1 + m_engine->cols()) % m_engine->cols());
        update();
        return;
    case Qt::Key_Right:
        m_cursorCell.setX((m_cursorCell.x() + 1) % m_engine->cols());
        update();
        return;
    case Qt::Key_Up:
        m_cursorCell.setY((m_cursorCell.y() - 1 + m_engine->rows()) % m_engine->rows());
        update();
        return;
    case Qt::Key_Down:
        m_cursorCell.setY((m_cursorCell.y() + 1) % m_engine->rows());
        update();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        m_engine->placePipeAt(m_cursorCell.x(), m_cursorCell.y());
        return;
    default:
        QWidget::keyPressEvent(event);
    }
}

} // namespace ValveRush
