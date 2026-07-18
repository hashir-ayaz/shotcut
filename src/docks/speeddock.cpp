/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "speeddock.h"

#include "docks/timelinedock.h"
#include "mainwindow.h"
#include "services/editorservice.h"
#include "services/speedpresets.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

SpeedDock::SpeedDock(QWidget *parent)
    : QDockWidget(tr("Speed"), parent)
{
    setObjectName(QStringLiteral("SpeedDock"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    QWidget *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);

    m_presetList = new QListWidget(container);
    for (const SpeedPreset &preset : SpeedPresets::all()) {
        auto *item = new QListWidgetItem(preset.name, m_presetList);
        item->setData(Qt::UserRole, preset.id);
    }
    m_presetList->setCurrentRow(0);
    layout->addWidget(m_presetList);

    auto *speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel(tr("Speed:"), container));
    m_speedSpinBox = new QDoubleSpinBox(container);
    m_speedSpinBox->setRange(0.1, 10.0);
    m_speedSpinBox->setValue(1.0);
    m_speedSpinBox->setSuffix(QStringLiteral(" x"));
    speedLayout->addWidget(m_speedSpinBox);
    layout->addLayout(speedLayout);

    m_pitchCheckBox = new QCheckBox(tr("Compensate pitch"), container);
    layout->addWidget(m_pitchCheckBox);

    auto *applyPresetButton = new QPushButton(tr("Apply Preset to Selected Clip"), container);
    connect(applyPresetButton, &QPushButton::clicked, this, &SpeedDock::onApplyPresetClicked);
    layout->addWidget(applyPresetButton);

    auto *applyConstantButton = new QPushButton(tr("Apply Constant Speed to Selected Clip"), container);
    connect(applyConstantButton, &QPushButton::clicked, this, &SpeedDock::onApplyConstantClicked);
    layout->addWidget(applyConstantButton);

    container->setLayout(layout);
    setWidget(container);
}

void SpeedDock::onApplyPresetClicked()
{
    EditorService *service = MAIN.editorService();
    if (!service)
        return;

    const QListWidgetItem *item = m_presetList->currentItem();
    if (!item)
        return;

    TimelineDock *timelineDock = MAIN.timelineDock();
    const QList<QPoint> selection = timelineDock->selection();
    if (selection.isEmpty()) {
        MAIN.showStatusMessage(tr("Select a clip to apply a speed preset."));
        return;
    }

    const QPoint point = selection.first();
    const int trackIndex = point.y();
    const int clipIndex = point.x();
    const QString presetId = item->data(Qt::UserRole).toString();
    const bool pitchCompensation = m_pitchCheckBox->isChecked();
    const EditorResult result = service->applySpeedPreset(trackIndex,
                                                            clipIndex,
                                                            presetId,
                                                            pitchCompensation);
    if (!result.ok)
        MAIN.showStatusMessage(result.error);
    else
        MAIN.showStatusMessage(tr("Speed preset applied."));
}

void SpeedDock::onApplyConstantClicked()
{
    EditorService *service = MAIN.editorService();
    if (!service)
        return;

    TimelineDock *timelineDock = MAIN.timelineDock();
    const QList<QPoint> selection = timelineDock->selection();
    if (selection.isEmpty()) {
        MAIN.showStatusMessage(tr("Select a clip to apply constant speed."));
        return;
    }

    const QPoint point = selection.first();
    const int trackIndex = point.y();
    const int clipIndex = point.x();
    const bool pitchCompensation = m_pitchCheckBox->isChecked();
    const EditorResult result = service->applyConstantSpeed(trackIndex,
                                                              clipIndex,
                                                              m_speedSpinBox->value(),
                                                              pitchCompensation);
    if (!result.ok)
        MAIN.showStatusMessage(result.error);
    else
        MAIN.showStatusMessage(tr("Constant speed applied."));
}
