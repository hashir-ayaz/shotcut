/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef ANIMATIONPRESETS_H
#define ANIMATIONPRESETS_H

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct AnimationKeyframeSpec
{
    QString propertyId;
    double startValue{0.0};
    double endValue{0.0};
};

struct AnimationPreset
{
    QString id;
    QString name;
    QStringList properties;
};

class AnimationPresets
{
public:
    static const QVector<AnimationPreset> &all();
    static const AnimationPreset *find(const QString &presetId);
    static QJsonObject toJson(const AnimationPreset &preset);
    static QVector<AnimationKeyframeSpec> keyframeSpecsFor(const AnimationPreset &preset,
                                                         bool isIn,
                                                         int profileWidth,
                                                         int profileHeight);
};

#endif // ANIMATIONPRESETS_H
