/*
    Copyright 2014 Paul Colby

    This file is part of Bipolar.

    Bipolar is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Biplar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Bipolar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "resultspage.h"

#include <QDebug>
#include <QDir>
#include <QProgressBar>
#include <QPushButton>
#include <QRegExp>
#include <QSettings>
#include <QTextEdit>
#include <QVBoxLayout>

ResultsPage::ResultsPage(QWidget *parent) : QWizardPage(parent)
{
    setTitle(tr("Converting..."));
    setButtonText(QWizard::FinishButton, tr("Close"));

    progressBar = new QProgressBar();

    showDetailsButton = new QPushButton(tr("Show details"));

    detailsBox = new QTextEdit();
    detailsBox->setReadOnly(true);
    detailsBox->setVisible(false);

    QVBoxLayout * const vBox = new QVBoxLayout;
    vBox->addWidget(progressBar);
    vBox->addWidget(showDetailsButton);
    vBox->addWidget(detailsBox);
    setLayout(vBox);

    connect(showDetailsButton, SIGNAL(clicked()), this, SLOT(showDetails()));
}

void ResultsPage::initializePage()
{
    QSettings settings;

    QRegExp regex(QLatin1String("(v2-users-[^-]+-training-sessions-[^-]+)-.*"));
    foreach (const QString &folder,
             settings.value(QLatin1String("inputFolders")).toStringList()) {
        QDir dir(folder);
        foreach (const QFileInfo &info, dir.entryInfoList()) {
            if (regex.exactMatch(info.fileName())) {
                const QString baseName = dir.absoluteFilePath(regex.cap());
                if (!sessionBaseNames.contains(baseName)) {
                    sessionBaseNames.append(baseName);
                }
            }
        }
    }

    progressBar->setRange(0, sessionBaseNames.size());
    progressBar->reset();
}

bool ResultsPage::isComplete() const
{
    return false;
}

// Protected slots.

void ResultsPage::showDetails()
{
    showDetailsButton->setVisible(false);
    detailsBox->setVisible(true);
}