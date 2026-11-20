/*
 * SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mainwindow.h"

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("valverush"));

    KAboutData aboutData(QStringLiteral("valverush"),
                         i18n("ValveRush"),
                         QStringLiteral("1.0.0"),
                         i18n("Connect pipes before the pressurized fluid leaks!"),
                         KAboutLicense::GPL_V2,
                         i18n("Copyright (C) 2026 Huberto Gastal Mayer"),
                         i18n("A Pipe Mania inspired puzzle game for KDE."));
    aboutData.setOrganizationDomain(QByteArrayLiteral("kde.org"));
    aboutData.setDesktopFileName(QStringLiteral("org.kde.valverush"));
    aboutData.addAuthor(i18n("Huberto Gastal Mayer"), i18n("Developer"), QStringLiteral("hubertogm@gmail.com"));

    KAboutData::setApplicationData(aboutData);
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("valverush")));

    KCrash::initialize();

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService service;

    auto *window = new ValveRush::MainWindow();
    window->show();

    return app.exec();
}
