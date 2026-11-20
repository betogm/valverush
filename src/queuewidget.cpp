/*
 * SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "queuewidget.h"

#include <KLocalizedString>
#include <QFrame>
#include <QPainter>
#include <QVBoxLayout>

namespace ValveRush
{

QueuePreviewWidget::QueuePreviewWidget(GameEngine *engine, QWidget *parent)
    : QWidget(parent)
    , m_engine(engine)
{
    setMinimumSize(96, 300);
    setAccessibleName(i18n("Next Pipes Queue"));
    connect(m_engine, &GameEngine::queueChanged, this, qOverload<>(&QWidget::update));
}

void QueuePreviewWidget::setThemePalette(const ThemePalette &palette)
{
    m_palette = palette;
    update();
}

QSize QueuePreviewWidget::sizeHint() const
{
    return QSize(110, 340);
}

void QueuePreviewWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), m_palette.boardBg);

    const QVector<PipeType> &q = m_engine->queue();
    if (q.isEmpty()) {
        return;
    }

    const qreal slotSize = qMin(width() - 16.0, (height() - 16.0) / GameEngine::QueueSize);
    const qreal left = (width() - slotSize) / 2.0;

    for (int i = 0; i < q.size(); ++i) {
        const QRectF slotRect(left, 8.0 + i * slotSize, slotSize, slotSize);
        const QRectF inner = slotRect.adjusted(4, 4, -4, -4);
        painter.fillRect(inner, m_palette.cellBg);

        if (i == 0) {
            painter.setPen(QPen(m_palette.fluidColor, 2.5));
        } else {
            painter.setPen(QPen(m_palette.cellBorder, 1.0));
        }
        painter.drawRoundedRect(inner, 6.0, 6.0);

        PipeRenderer::drawPipePreview(painter, inner, q[i], m_palette);
    }
}

SidePanelWidget::SidePanelWidget(GameEngine *engine, QWidget *parent)
    : QWidget(parent)
    , m_engine(engine)
{
    setFixedWidth(185);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto makeStatLabel = [&](const QString &initial) {
        auto *lbl = new QLabel(initial, this);
        QFont f = lbl->font();
        f.setBold(true);
        lbl->setFont(f);
        lbl->setAlignment(Qt::AlignCenter);
        return lbl;
    };

    m_levelLabel = makeStatLabel(i18n("Level: 1"));
    m_scoreLabel = makeStatLabel(i18n("Score: 0"));
    m_pipesLabel = makeStatLabel(i18n("Pipes: 0 / 10"));
    m_timerLabel = new QLabel(i18n("Valve opens in:"), this);
    m_timerLabel->setAlignment(Qt::AlignCenter);

    m_timerBar = new QProgressBar(this);
    m_timerBar->setRange(0, 100);
    m_timerBar->setValue(100);
    m_timerBar->setTextVisible(true);

    auto *queueTitle = new QLabel(i18n("Next Pipes"), this);
    queueTitle->setAlignment(Qt::AlignCenter);
    QFont qf = queueTitle->font();
    qf.setBold(true);
    queueTitle->setFont(qf);

    m_queuePreview = new QueuePreviewWidget(m_engine, this);

    m_fastForwardBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("media-seek-forward")), i18n("Fast Flow"), this);
    m_fastForwardBtn->setCheckable(true);
    m_fastForwardBtn->setToolTip(i18n("Accelerate fluid flow for bonus points (Shortcut: F)"));
    connect(m_fastForwardBtn, &QPushButton::toggled, m_engine, &GameEngine::setFastForward);

    m_nextLevelBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("go-next")), i18n("Next Level"), this);
    m_nextLevelBtn->setVisible(false);
    connect(m_nextLevelBtn, &QPushButton::clicked, this, [this]() {
        m_engine->startLevel(m_engine->level() + 1);
    });

    layout->addWidget(m_levelLabel);
    layout->addWidget(m_scoreLabel);
    layout->addWidget(m_pipesLabel);

    auto *sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    layout->addWidget(sep);

    layout->addWidget(m_timerLabel);
    layout->addWidget(m_timerBar);
    layout->addWidget(queueTitle);
    layout->addWidget(m_queuePreview, 1);
    layout->addWidget(m_fastForwardBtn);
    layout->addWidget(m_nextLevelBtn);

    connect(m_engine, &GameEngine::statsChanged, this, &SidePanelWidget::updateStats);
    connect(m_engine, &GameEngine::phaseChanged, this, &SidePanelWidget::updatePhase);
    updateStats();
    updatePhase(m_engine->phase());
}

void SidePanelWidget::setThemePalette(const ThemePalette &palette)
{
    m_queuePreview->setThemePalette(palette);
}

void SidePanelWidget::updateStats()
{
    m_levelLabel->setText(i18n("Level: %1", m_engine->level()));
    m_scoreLabel->setText(i18n("Score: %1", m_engine->score()));
    m_pipesLabel->setText(i18n("Pipes: %1 / %2", m_engine->pipesFilled(), m_engine->targetPipes()));

    if (m_engine->phase() == GamePhase::PreFlow && m_engine->preFlowTotal() > 0.0) {
        const int pct = qRound((m_engine->preFlowRemaining() / m_engine->preFlowTotal()) * 100.0);
        m_timerBar->setValue(pct);
        m_timerBar->setFormat(i18n("%1 s", QString::number(m_engine->preFlowRemaining(), 'f', 1)));
    } else if (m_engine->phase() == GamePhase::Flowing) {
        m_timerBar->setValue(0);
        m_timerBar->setFormat(i18n("Flowing!"));
    }

    if (m_fastForwardBtn->isChecked() != m_engine->isFastForward()) {
        m_fastForwardBtn->setChecked(m_engine->isFastForward());
    }
}

void SidePanelWidget::updatePhase(GamePhase phase)
{
    const bool active = (phase == GamePhase::PreFlow || phase == GamePhase::Flowing);
    m_fastForwardBtn->setEnabled(active);
    m_nextLevelBtn->setVisible(phase == GamePhase::LevelWon);

    if (phase == GamePhase::PreFlow) {
        m_timerLabel->setText(i18n("Valve opens in:"));
    } else if (phase == GamePhase::Flowing) {
        m_timerLabel->setText(i18n("Fluid active"));
    } else if (phase == GamePhase::Paused) {
        m_timerLabel->setText(i18n("Paused"));
    } else if (phase == GamePhase::LevelWon) {
        m_timerLabel->setText(i18n("Level Complete!"));
    } else if (phase == GamePhase::GameOver) {
        m_timerLabel->setText(i18n("Leak Detected!"));
    }
}

} // namespace ValveRush
