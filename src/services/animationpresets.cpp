/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "animationpresets.h"

#include <QJsonArray>

static QVector<AnimationPreset> makePresets()
{
    return {
        {QStringLiteral("fade"),
         QStringLiteral("Fade"),
         {QStringLiteral("opacity")}},
        {QStringLiteral("zoom_in"),
         QStringLiteral("Zoom In"),
         {QStringLiteral("scale")}},
        {QStringLiteral("zoom_out"),
         QStringLiteral("Zoom Out"),
         {QStringLiteral("scale")}},
        {QStringLiteral("slide_left"),
         QStringLiteral("Slide Left"),
         {QStringLiteral("position.x")}},
        {QStringLiteral("slide_up"),
         QStringLiteral("Slide Up"),
         {QStringLiteral("position.y")}},
        {QStringLiteral("fade_zoom"),
         QStringLiteral("Fade + Zoom"),
         {QStringLiteral("opacity"), QStringLiteral("scale")}},
    };
}

const QVector<AnimationPreset> &AnimationPresets::all()
{
    static const QVector<AnimationPreset> presets = makePresets();
    return presets;
}

const AnimationPreset *AnimationPresets::find(const QString &presetId)
{
    for (const AnimationPreset &preset : all()) {
        if (preset.id == presetId)
            return &preset;
    }
    return nullptr;
}

QJsonObject AnimationPresets::toJson(const AnimationPreset &preset)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), preset.id);
    object.insert(QStringLiteral("name"), preset.name);
    QJsonArray properties;
    for (const QString &property : preset.properties)
        properties.append(property);
    object.insert(QStringLiteral("properties"), properties);
    object.insert(QStringLiteral("modes"),
                  QJsonArray{QStringLiteral("in"), QStringLiteral("out"), QStringLiteral("combo")});
    return object;
}

QVector<AnimationKeyframeSpec> AnimationPresets::keyframeSpecsFor(const AnimationPreset &preset,
                                                                  bool isIn,
                                                                  int profileWidth,
                                                                  int profileHeight)
{
    const QString opacity = QStringLiteral("opacity");
    const QString scale = QStringLiteral("scale");
    const QString positionX = QStringLiteral("position.x");
    const QString positionY = QStringLiteral("position.y");

    if (preset.id == QStringLiteral("fade")) {
        if (isIn)
            return {{opacity, 0.0, 1.0}};
        return {{opacity, 1.0, 0.0}};
    }
    if (preset.id == QStringLiteral("zoom_in")) {
        if (isIn)
            return {{scale, 0.7, 1.0}};
        return {{scale, 1.0, 1.3}};
    }
    if (preset.id == QStringLiteral("zoom_out")) {
        if (isIn)
            return {{scale, 1.3, 1.0}};
        return {{scale, 1.0, 0.7}};
    }
    if (preset.id == QStringLiteral("slide_left")) {
        const double width = static_cast<double>(profileWidth);
        if (isIn)
            return {{positionX, width, 0.0}};
        return {{positionX, 0.0, -width}};
    }
    if (preset.id == QStringLiteral("slide_up")) {
        const double height = static_cast<double>(profileHeight);
        if (isIn)
            return {{positionY, height, 0.0}};
        return {{positionY, 0.0, -height}};
    }
    if (preset.id == QStringLiteral("fade_zoom")) {
        if (isIn)
            return {{opacity, 0.0, 1.0}, {scale, 0.85, 1.0}};
        return {{opacity, 1.0, 0.0}, {scale, 1.0, 1.15}};
    }
    return {};
}
