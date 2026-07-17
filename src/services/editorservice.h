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

    Q_INVOKABLE EditorResult listTransitionPresets();
    Q_INVOKABLE EditorResult applyTransition(int trackIndex,
                                             int clipIndex,
                                             const QString &presetId,
                                             int durationFrames);
    Q_INVOKABLE EditorResult setTransitionDuration(int trackIndex,
                                                   int transitionClipIndex,
                                                   int durationFrames);
    Q_INVOKABLE EditorResult getTransition(int trackIndex, int transitionClipIndex);
    Q_INVOKABLE EditorResult removeTransition(int trackIndex, int transitionClipIndex);

    Q_INVOKABLE EditorResult listAnimatableProperties(int trackIndex, int clipIndex);
    Q_INVOKABLE EditorResult addKeyframe(int trackIndex,
                                         int clipIndex,
                                         const QString &propertyId,
                                         int frame,
                                         double value,
                                         const QString &interpolation);
    Q_INVOKABLE EditorResult removeKeyframe(int trackIndex,
                                            int clipIndex,
                                            const QString &propertyId,
                                            int frame);
    Q_INVOKABLE EditorResult listKeyframes(int trackIndex,
                                           int clipIndex,
                                           const QString &propertyId);
    Q_INVOKABLE EditorResult setPropertyValue(int trackIndex,
                                              int clipIndex,
                                              const QString &propertyId,
                                              double value);

    Q_INVOKABLE EditorResult transcribeCaptions(const QString &language,
                                                bool translateToEnglish,
                                                const QString &trackName = QString());
    Q_INVOKABLE EditorResult getJobStatus(const QString &jobId);
    Q_INVOKABLE EditorResult getCaptions();
    Q_INVOKABLE EditorResult setCaptionText(int trackIndex, int cueIndex, const QString &text);
    Q_INVOKABLE EditorResult importCaptionsFile(const QString &srtPath,
                                                const QString &trackName = QString());

private:
    MainWindow *m_mainWindow;
    QString m_lastJobLabel;
};

#endif // EDITORSERVICE_H
