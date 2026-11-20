/*
 * SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mainwindow.h"
#include "valverush.h"

#include <KActionCollection>
#include <KGameHighScoreDialog>
#include <KGameStandardAction>
#include <KGameTheme>
#include <KGameThemeSelector>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardAction>
#include <KToggleAction>
#include <QDialog>
#include <QHBoxLayout>
#include <QStatusBar>

namespace ValveRush
{

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
    , m_engine(new GameEngine(this))
    , m_difficulty(new KGameDifficulty(this))
    , m_themeProvider(new KGameThemeProvider(QByteArrayLiteral("Theme"), this))
{
    m_themeProvider->discoverThemes(QStringLiteral("themes"), QStringLiteral("breeze"));

    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    m_boardView = new BoardView(m_engine, central);
    m_sidePanel = new SidePanelWidget(m_engine, central);

    layout->addWidget(m_boardView, 1);
    layout->addWidget(m_sidePanel, 0);
    setCentralWidget(central);

    setupDifficulty();
    setupActions();
    setupGUI(Default, QStringLiteral("valverushui.rc"));

    connect(m_themeProvider, &KGameThemeProvider::currentThemeChanged, this, &MainWindow::onThemeChanged);
    onThemeChanged(m_themeProvider->currentTheme());

    m_boardView->setShowCoordinates(Settings::showGridCoordinates());

    connect(m_engine, &GameEngine::levelCompleted, this, &MainWindow::onLevelCompleted);
    connect(m_engine, &GameEngine::gameOver, this, &MainWindow::onGameOver);
    connect(m_engine, &GameEngine::statsChanged, this, [this]() {
        statusBar()->showMessage(
            i18n("Level %1 | Pipes: %2/%3 | Score: %4", m_engine->level(), m_engine->pipesFilled(), m_engine->targetPipes(), m_engine->score()));
    });

    m_engine->startNewGame();
}

void MainWindow::setupDifficulty()
{
    m_difficulty->addStandardLevel(KGameDifficultyLevel::Easy);
    m_difficulty->addStandardLevel(KGameDifficultyLevel::Medium, true);
    m_difficulty->addStandardLevel(KGameDifficultyLevel::Hard);
    KGameDifficultyGUI::init(this, m_difficulty);
    connect(m_difficulty, &KGameDifficulty::currentLevelChanged, this, &MainWindow::onDifficultyChanged);
}

void MainWindow::setupActions()
{
    KGameStandardAction::gameNew(this, &MainWindow::onNewGame, actionCollection());
    m_pauseAction = KGameStandardAction::pause(this, &MainWindow::onPauseGame, actionCollection());
    KGameStandardAction::highscores(this, &MainWindow::onShowHighScores, actionCollection());
    KGameStandardAction::quit(this, &QWidget::close, actionCollection());

    m_fastForwardAction = new QAction(QIcon::fromTheme(QStringLiteral("media-seek-forward")), i18n("Fast Forward Flow"), this);
    actionCollection()->addAction(QStringLiteral("fast_forward"), m_fastForwardAction);
    actionCollection()->setDefaultShortcut(m_fastForwardAction, QKeySequence(Qt::Key_F));
    m_fastForwardAction->setCheckable(true);
    connect(m_fastForwardAction, &QAction::toggled, m_engine, &GameEngine::setFastForward);

    auto *themeAction = new QAction(QIcon::fromTheme(QStringLiteral("preferences-desktop-theme")), i18n("Configure Theme..."), this);
    actionCollection()->addAction(QStringLiteral("configure_theme"), themeAction);
    connect(themeAction, &QAction::triggered, this, &MainWindow::onConfigureTheme);

    auto *coordsAction = new KToggleAction(i18n("Show Grid Coordinates"), this);
    coordsAction->setChecked(Settings::showGridCoordinates());
    actionCollection()->addAction(QStringLiteral("show_coordinates"), coordsAction);
    connect(coordsAction, &QAction::toggled, this, &MainWindow::onToggleCoordinates);
}

void MainWindow::onNewGame()
{
    m_pauseAction->setChecked(false);
    m_fastForwardAction->setChecked(false);
    m_engine->startNewGame();
}

void MainWindow::onPauseGame(bool /*paused*/)
{
    m_engine->togglePause();
}

void MainWindow::onShowHighScores()
{
    KGameHighScoreDialog dlg(KGameHighScoreDialog::Name | KGameHighScoreDialog::Score | KGameHighScoreDialog::Level, this);
    dlg.initFromDifficulty(m_difficulty);
    dlg.exec();
}

void MainWindow::onConfigureTheme()
{
    auto *dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(i18n("Configure Theme — ValveRush"));
    auto *lay = new QHBoxLayout(dlg);
    auto *selector = new KGameThemeSelector(m_themeProvider, KGameThemeSelector::DefaultBehavior, dlg);
    lay->addWidget(selector);
    dlg->resize(540, 380);
    dlg->show();
}

void MainWindow::onDifficultyChanged(const KGameDifficultyLevel *level)
{
    if (!level) {
        return;
    }
    switch (level->standardLevel()) {
    case KGameDifficultyLevel::Easy:
        m_engine->setDifficulty(Difficulty::Easy);
        break;
    case KGameDifficultyLevel::Hard:
        m_engine->setDifficulty(Difficulty::Hard);
        break;
    default:
        m_engine->setDifficulty(Difficulty::Medium);
        break;
    }
}

void MainWindow::onThemeChanged(const KGameTheme *theme)
{
    const QString id = theme ? QString::fromUtf8(theme->identifier()) : QStringLiteral("breeze");
    const ThemePalette palette = ThemePalette::fromThemeId(id);
    m_boardView->setThemePalette(palette);
    m_sidePanel->setThemePalette(palette);
}

void MainWindow::onLevelCompleted(int level, int bonusPoints)
{
    KMessageBox::information(this,
                             i18n("Level %1 cleared!\nBonus points: +%2\n\nClick \"Next Level\" to continue.", level, bonusPoints),
                             i18n("Level Complete"));
}

void MainWindow::onGameOver(int finalScore)
{
    if (finalScore > 0) {
        KGameHighScoreDialog dlg(KGameHighScoreDialog::Name | KGameHighScoreDialog::Score | KGameHighScoreDialog::Level, this);
        dlg.initFromDifficulty(m_difficulty);

        KGameHighScoreDialog::FieldInfo scoreInfo;
        scoreInfo[KGameHighScoreDialog::Score] = QString::number(finalScore);
        scoreInfo[KGameHighScoreDialog::Level] = QString::number(m_engine->level());

        if (dlg.addScore(scoreInfo, KGameHighScoreDialog::AskName)) {
            dlg.exec();
            return;
        }
    }

    KMessageBox::information(this, i18n("The fluid leaked before reaching the target!\nFinal score: %1", finalScore), i18n("Game Over"));
}

void MainWindow::onToggleCoordinates(bool show)
{
    Settings::setShowGridCoordinates(show);
    Settings::self()->save();
    m_boardView->setShowCoordinates(show);
}

} // namespace ValveRush
