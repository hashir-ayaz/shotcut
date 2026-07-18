/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "audiodock.h"

#include "docks/timelinedock.h"
#include "mainwindow.h"
#include "services/editorservice.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

AudioDock::AudioDock(QWidget *parent)
    : QDockWidget(tr("Audio"), parent)
{
    setObjectName(QStringLiteral("AudioDock"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    QWidget *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);

    auto *amountLayout = new QHBoxLayout();
    amountLayout->addWidget(new QLabel(tr("Amount:"), container));
    m_amountSpinBox = new QSpinBox(container);
    m_amountSpinBox->setRange(0, 100);
    m_amountSpinBox->setValue(100);
    m_amountSpinBox->setSuffix(QStringLiteral(" %"));
    amountLayout->addWidget(m_amountSpinBox);
    layout->addLayout(amountLayout);

    auto *applyButton = new QPushButton(tr("Reduce Noise on Selected Clip"), container);
    connect(applyButton, &QPushButton::clicked, this, &AudioDock::onApplyClicked);
    layout->addWidget(applyButton);

    container->setLayout(layout);
    setWidget(container);
}

void AudioDock::onApplyClicked()
{
    EditorService *service = MAIN.editorService();
    if (!service)
        return;

    TimelineDock *timelineDock = MAIN.timelineDock();
    const QList<QPoint> selection = timelineDock->selection();
    if (selection.isEmpty()) {
        MAIN.showStatusMessage(tr("Select a clip to reduce noise."));
        return;
    }

    const QPoint point = selection.first();
    const int trackIndex = point.y();
    const int clipIndex = point.x();
    const double amount = m_amountSpinBox->value() / 100.0;
    const EditorResult result = service->applyAudioDenoise(trackIndex, clipIndex, amount);
    if (!result.ok)
        MAIN.showStatusMessage(result.error);
    else
        MAIN.showStatusMessage(tr("Noise reduction applied."));
}
