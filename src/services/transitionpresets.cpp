/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "transitionpresets.h"

static QVector<TransitionPreset> makePresets()
{
    return {
        {QStringLiteral("dissolve"), QStringLiteral("Dissolve"), QStringLiteral("dissolve"), QString()},
        {QStringLiteral("wipe_bar_horizontal"),
         QStringLiteral("Bar Horizontal"),
         QStringLiteral("wipe"),
         QStringLiteral("%luma01.pgm")},
        {QStringLiteral("wipe_bar_vertical"),
         QStringLiteral("Bar Vertical"),
         QStringLiteral("wipe"),
         QStringLiteral("%luma02.pgm")},
        {QStringLiteral("wipe_barn_door_horizontal"),
         QStringLiteral("Barn Door Horizontal"),
         QStringLiteral("wipe"),
         QStringLiteral("%luma03.pgm")},
        {QStringLiteral("wipe_barn_door_vertical"),
         QStringLiteral("Barn Door Vertical"),
         QStringLiteral("wipe"),
         QStringLiteral("%luma04.pgm")},
        {QStringLiteral("wipe_barn_door_diagonal_sw_ne"),
         QStringLiteral("Barn Door Diagonal SW-NE"),
         QStringLiteral("wipe"),
         QStringLiteral("%luma05.pgm")},
        {QStringLiteral("wipe_barn_door_diagonal_nw_se"),
         QStringLiteral("Barn Door Diagonal NW-SE"),
         QStringLiteral("wipe"),
         QStringLiteral("%luma06.pgm")},
        {QStringLiteral("wipe_diagonal_top_left"),
         QStringLiteral("Diagonal Top Left"),
         QStringLiteral("wipe"),
         QStringLiteral("%luma07.pgm")},
        {QStringLiteral("wipe_diagonal_top_right"),
         QStringLiteral("Diagonal Top Right"),
         QStringLiteral("wipe"),
         QStringLiteral("%luma08.pgm")},
        {QStringLiteral("wipe_iris_circle"),
         QStringLiteral("Iris Circle"),
         QStringLiteral("wipe"),
         QStringLiteral("%luma15.pgm")},
        {QStringLiteral("wipe_double_iris"),
         QStringLiteral("Double Iris"),
         QStringLiteral("wipe"),
         QStringLiteral("%luma16.pgm")},
        {QStringLiteral("wipe_iris_box"),
         QStringLiteral("Iris Box"),
         QStringLiteral("wipe"),
         QStringLiteral("%luma17.pgm")},
        {QStringLiteral("wipe_clock_top"),
         QStringLiteral("Clock Top"),
         QStringLiteral("wipe"),
         QStringLiteral("%luma20.pgm")},
    };
}

const QVector<TransitionPreset> &TransitionPresets::all()
{
    static const QVector<TransitionPreset> presets = makePresets();
    return presets;
}

const TransitionPreset *TransitionPresets::find(const QString &presetId)
{
    for (const TransitionPreset &preset : all()) {
        if (preset.id == presetId)
            return &preset;
    }
    return nullptr;
}

QJsonObject TransitionPresets::toJson(const TransitionPreset &preset)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), preset.id);
    object.insert(QStringLiteral("name"), preset.name);
    object.insert(QStringLiteral("family"), preset.family);
    object.insert(QStringLiteral("resource"), preset.resource);
    object.insert(QStringLiteral("softness"), preset.softness);
    object.insert(QStringLiteral("progressive"), preset.progressive);
    object.insert(QStringLiteral("mltService"), QStringLiteral("luma"));
    return object;
}
