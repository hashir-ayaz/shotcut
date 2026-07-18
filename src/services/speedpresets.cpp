/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "speedpresets.h"

#include <QJsonArray>

static QVector<SpeedPreset> makePresets()
{
    return {
        {QStringLiteral("montage"),
         QStringLiteral("Montage"),
         QStringLiteral("smooth"),
         {{0.0, 1.0}, {0.5, 2.0}, {1.0, 1.0}}},
        {QStringLiteral("hero"),
         QStringLiteral("Hero"),
         QStringLiteral("smooth"),
         {{0.0, 2.0}, {0.4, 0.4}, {0.6, 0.4}, {1.0, 2.0}}},
        {QStringLiteral("bullet"),
         QStringLiteral("Bullet"),
         QStringLiteral("smooth"),
         {{0.0, 1.0}, {0.45, 0.2}, {0.5, 0.2}, {0.55, 0.2}, {1.0, 1.0}}},
        {QStringLiteral("jump_cut"),
         QStringLiteral("Jump Cut"),
         QStringLiteral("discrete"),
         {{0.0, 1.0}, {0.5, 4.0}, {1.0, 1.0}}},
        {QStringLiteral("flash_in"),
         QStringLiteral("Flash In"),
         QStringLiteral("smooth"),
         {{0.0, 4.0}, {0.25, 1.0}, {1.0, 1.0}}},
        {QStringLiteral("flash_out"),
         QStringLiteral("Flash Out"),
         QStringLiteral("smooth"),
         {{0.0, 1.0}, {0.75, 1.0}, {1.0, 4.0}}},
    };
}

const QVector<SpeedPreset> &SpeedPresets::all()
{
    static const QVector<SpeedPreset> presets = makePresets();
    return presets;
}

const SpeedPreset *SpeedPresets::find(const QString &presetId)
{
    for (const SpeedPreset &preset : all()) {
        if (preset.id == presetId)
            return &preset;
    }
    return nullptr;
}

QJsonObject SpeedPresets::toJson(const SpeedPreset &preset)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), preset.id);
    object.insert(QStringLiteral("name"), preset.name);
    object.insert(QStringLiteral("interpolation"), preset.interpolation);
    QJsonArray keyframes;
    for (const SpeedKeyframe &keyframe : preset.keyframes) {
        QJsonObject keyframeObject;
        keyframeObject.insert(QStringLiteral("position"), keyframe.position);
        keyframeObject.insert(QStringLiteral("speed"), keyframe.speed);
        keyframes.append(keyframeObject);
    }
    object.insert(QStringLiteral("keyframes"), keyframes);
    return object;
}
