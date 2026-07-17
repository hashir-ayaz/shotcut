/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TRANSITIONPRESETS_H
#define TRANSITIONPRESETS_H

#include <QJsonObject>
#include <QString>
#include <QVector>

struct TransitionPreset
{
    QString id;
    QString name;
    QString family;
    QString resource;
    double softness{0.2};
    bool progressive{true};
};

class TransitionPresets
{
public:
    static const QVector<TransitionPreset> &all();
    static const TransitionPreset *find(const QString &presetId);
    static QJsonObject toJson(const TransitionPreset &preset);
};

#endif // TRANSITIONPRESETS_H
