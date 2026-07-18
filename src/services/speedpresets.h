/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SPEEDPRESETS_H
#define SPEEDPRESETS_H

#include <QJsonObject>
#include <QString>
#include <QVector>

struct SpeedKeyframe {
    double position{0.0};  // 0.0..1.0 of clip duration
    double speed{1.0};
};

struct SpeedPreset {
    QString id;
    QString name;
    QString interpolation;  // "smooth" | "linear" | "discrete"
    QVector<SpeedKeyframe> keyframes;
};

class SpeedPresets
{
public:
    static const QVector<SpeedPreset> &all();
    static const SpeedPreset *find(const QString &presetId);
    static QJsonObject toJson(const SpeedPreset &preset);
};

#endif // SPEEDPRESETS_H
