/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef EDITORTYPES_H
#define EDITORTYPES_H

#include <QJsonObject>
#include <QString>

struct EditorResult
{
    bool ok{false};
    QString error;
    QJsonObject data;

    static EditorResult success(const QJsonObject &data = {})
    {
        EditorResult result;
        result.ok = true;
        result.data = data;
        return result;
    }

    static EditorResult failure(const QString &error)
    {
        EditorResult result;
        result.ok = false;
        result.error = error;
        return result;
    }

    QJsonObject toJsonObject() const
    {
        QJsonObject object;
        object.insert(QStringLiteral("ok"), ok);
        if (!error.isEmpty())
            object.insert(QStringLiteral("error"), error);
        if (!data.isEmpty())
            object.insert(QStringLiteral("data"), data);
        return object;
    }
};

#endif // EDITORTYPES_H
