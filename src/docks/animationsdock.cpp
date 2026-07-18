/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "animationsdock.h"

#include "docks/timelinedock.h"
#include "mainwindow.h"
#include "services/editorservice.h"
#include "services/animationpresets.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

AnimationsDock::AnimationsDock(QWidget *parent)
    : QDockWidget(tr("Animations"), parent)
{
    setObjectName(QStringLiteral("AnimationsDock"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    QWidget *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);

    auto *modeLayout = new QHBoxLayout();
    modeLayout->addWidget(new QLabel(tr("Mode:"), container));
    m_modeCombo = new QComboBox(container);
    m_modeCombo->addItem(tr("In"), QStringLiteral("in"));
    m_modeCombo->addItem(tr("Out"), QStringLiteral("out"));
    m_modeCombo->addItem(tr("Combo"), QStringLiteral("combo"));
    modeLayout->addWidget(m_modeCombo);
    layout->addLayout(modeLayout);

    auto *durationLayout = new QHBoxLayout();
    durationLayout->addWidget(new QLabel(tr("Duration (frames):"), container));
    m_durationSpinBox = new QSpinBox(container);
    m_durationSpinBox->setRange(1, 10000);
    m_durationSpinBox->setValue(24);
    durationLayout->addWidget(m_durationSpinBox);
    layout->addLayout(durationLayout);

    m_presetList = new QListWidget(container);
    for (const AnimationPreset &preset : AnimationPresets::all()) {
        auto *item = new QListWidgetItem(preset.name, m_presetList);
        item->setData(Qt::UserRole, preset.id);
    }
    m_presetList->setCurrentRow(0);
    layout->addWidget(m_presetList);

    auto *applyButton = new QPushButton(tr("Apply to Selected Clip"), container);
    connect(applyButton, &QPushButton::clicked, this, &AnimationsDock::onApplyClicked);
    layout->addWidget(applyButton);

    container->setLayout(layout);
    setWidget(container);
}

void AnimationsDock::onApplyClicked()
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
        MAIN.showStatusMessage(tr("Select a clip to apply an animation."));
        return;
    }

    const QPoint point = selection.first();
    const int trackIndex = point.y();
    const int clipIndex = point.x();
    const QString presetId = item->data(Qt::UserRole).toString();
    const QString mode = m_modeCombo->currentData().toString();
    const EditorResult result = service->applyClipAnimation(trackIndex,
                                                            clipIndex,
                                                            presetId,
                                                            m_durationSpinBox->value(),
                                                            mode);
    if (!result.ok)
        MAIN.showStatusMessage(result.error);
    else
        MAIN.showStatusMessage(tr("Animation applied."));
}
