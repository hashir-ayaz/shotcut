/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef EDITORSERVICE_H
#define EDITORSERVICE_H

#include "editortypes.h"

#include <QObject>

class MainWindow;

class EditorService : public QObject
{
    Q_OBJECT

public:
    explicit EditorService(MainWindow *mainWindow, QObject *parent = nullptr);

    Q_INVOKABLE EditorResult ping();
    Q_INVOKABLE EditorResult getState();
    Q_INVOKABLE EditorResult openProject(const QString &path);
    Q_INVOKABLE EditorResult saveProject(const QString &path);
    Q_INVOKABLE EditorResult addClipToTimeline(const QString &path,
                                               int trackIndex = -1,
                                               int position = -1);

private:
    MainWindow *m_mainWindow;
};

#endif // EDITORSERVICE_H
