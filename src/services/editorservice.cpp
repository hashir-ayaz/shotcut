/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "editorservice.h"

#include "commands/timelinecommands.h"
#include "docks/timelinedock.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/multitrackmodel.h"

#include <QFileInfo>
#include <QJsonArray>

EditorService::EditorService(MainWindow *mainWindow, QObject *parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
{
}

EditorResult EditorService::ping()
{
    QJsonObject data;
    data.insert(QStringLiteral("pong"), true);
    data.insert(QStringLiteral("version"), QStringLiteral(SHOTCUT_VERSION));
    return EditorResult::success(data);
}

EditorResult EditorService::getState()
{
    QJsonObject data;
    data.insert(QStringLiteral("projectPath"), m_mainWindow->fileName());

    Mlt::Profile &profile = MLT.profile();
    QJsonObject profileObject;
    profileObject.insert(QStringLiteral("width"), profile.width());
    profileObject.insert(QStringLiteral("height"), profile.height());
    profileObject.insert(QStringLiteral("fps"), profile.fps());
    data.insert(QStringLiteral("profile"), profileObject);

    TimelineDock *timelineDock = m_mainWindow->timelineDock();
    data.insert(QStringLiteral("playhead"), timelineDock->position());
    data.insert(QStringLiteral("currentTrack"), timelineDock->currentTrack());
    data.insert(QStringLiteral("multitrackValid"), m_mainWindow->isMultitrackValid());

    QJsonArray tracksArray;
    if (m_mainWindow->isMultitrackValid()) {
        MultitrackModel *model = timelineDock->model();
        const TrackList &tracks = model->trackList();
        for (int trackIndex = 0; trackIndex < tracks.size(); ++trackIndex) {
            const Track &track = tracks.at(trackIndex);
            QJsonObject trackObject;
            trackObject.insert(QStringLiteral("index"), trackIndex);
            switch (track.type) {
            case VideoTrackType:
                trackObject.insert(QStringLiteral("type"), QStringLiteral("video"));
                break;
            case AudioTrackType:
                trackObject.insert(QStringLiteral("type"), QStringLiteral("audio"));
                break;
            case BlackTrackType:
                trackObject.insert(QStringLiteral("type"), QStringLiteral("black"));
                break;
            case SilentTrackType:
                trackObject.insert(QStringLiteral("type"), QStringLiteral("silent"));
                break;
            default:
                trackObject.insert(QStringLiteral("type"), QStringLiteral("other"));
                break;
            }
            trackObject.insert(QStringLiteral("number"), track.number);

            QJsonArray clipsArray;
            const int clipCount = timelineDock->clipCount(trackIndex);
            for (int clipIndex = 0; clipIndex < clipCount; ++clipIndex) {
                const QModelIndex clipModelIndex = model->makeIndex(trackIndex, clipIndex);
                QJsonObject clipObject;
                clipObject.insert(QStringLiteral("index"), clipIndex);
                clipObject.insert(QStringLiteral("name"),
                                  model->data(clipModelIndex, MultitrackModel::NameRole)
                                      .toString());
                clipObject.insert(QStringLiteral("resource"),
                                  model->data(clipModelIndex, MultitrackModel::ResourceRole)
                                      .toString());
                clipObject.insert(QStringLiteral("start"),
                                  model->data(clipModelIndex, MultitrackModel::StartRole).toInt());
                clipObject.insert(QStringLiteral("duration"),
                                  model->data(clipModelIndex, MultitrackModel::DurationRole)
                                      .toInt());
                clipObject.insert(QStringLiteral("inPoint"),
                                  model->data(clipModelIndex, MultitrackModel::InPointRole).toInt());
                clipObject.insert(QStringLiteral("outPoint"),
                                  model->data(clipModelIndex, MultitrackModel::OutPointRole).toInt());
                clipObject.insert(QStringLiteral("isTransition"),
                                  model->data(clipModelIndex, MultitrackModel::IsTransitionRole)
                                      .toBool());
                clipObject.insert(QStringLiteral("isBlank"),
                                  model->data(clipModelIndex, MultitrackModel::IsBlankRole).toBool());
                clipsArray.append(clipObject);
            }
            trackObject.insert(QStringLiteral("clips"), clipsArray);
            tracksArray.append(trackObject);
        }
    }
    data.insert(QStringLiteral("tracks"), tracksArray);

    return EditorResult::success(data);
}

EditorResult EditorService::openProject(const QString &path)
{
    if (path.isEmpty())
        return EditorResult::failure(QStringLiteral("path is required"));

    QFileInfo fileInfo(path);
    if (!fileInfo.exists())
        return EditorResult::failure(QStringLiteral("file does not exist: %1").arg(path));

    if (!m_mainWindow->open(path))
        return EditorResult::failure(QStringLiteral("failed to open: %1").arg(path));

    QJsonObject data;
    data.insert(QStringLiteral("projectPath"), m_mainWindow->fileName());
    return EditorResult::success(data);
}

EditorResult EditorService::saveProject(const QString &path)
{
    QString savePath = path;
    if (savePath.isEmpty()) {
        savePath = m_mainWindow->fileName();
        if (savePath.isEmpty())
            return EditorResult::failure(QStringLiteral("no project path; provide path"));
    }

    if (!m_mainWindow->saveXML(savePath))
        return EditorResult::failure(QStringLiteral("failed to save: %1").arg(savePath));

    QJsonObject data;
    data.insert(QStringLiteral("projectPath"), savePath);
    return EditorResult::success(data);
}

EditorResult EditorService::addClipToTimeline(const QString &path,
                                              int trackIndex,
                                              int position)
{
    if (path.isEmpty())
        return EditorResult::failure(QStringLiteral("path is required"));

    QFileInfo fileInfo(path);
    if (!fileInfo.exists())
        return EditorResult::failure(QStringLiteral("file does not exist: %1").arg(path));

    TimelineDock *timelineDock = m_mainWindow->timelineDock();
    MultitrackModel *model = timelineDock->model();
    if (!m_mainWindow->isMultitrackValid() && !model->createIfNeeded())
        return EditorResult::failure(QStringLiteral("failed to create timeline"));

    if (trackIndex < 0)
        trackIndex = timelineDock->currentTrack();
    if (position < 0)
        position = timelineDock->position();

    if (trackIndex < 0 || trackIndex >= model->trackList().size())
        return EditorResult::failure(QStringLiteral("invalid trackIndex: %1").arg(trackIndex));

    Mlt::Producer producer(MLT.profile(), path.toUtf8().constData());
    if (!producer.is_valid())
        return EditorResult::failure(QStringLiteral("failed to load media: %1").arg(path));

    Mlt::Producer *readyProducer = MLT.setupNewProducer(&producer);
    if (!readyProducer || !readyProducer->is_valid())
        return EditorResult::failure(QStringLiteral("failed to prepare media: %1").arg(path));

    const QString xml = MLT.XML(readyProducer);
    m_mainWindow->undoStack()->push(
        new Timeline::OverwriteCommand(*model, trackIndex, position, xml));

    QJsonObject data;
    data.insert(QStringLiteral("trackIndex"), trackIndex);
    data.insert(QStringLiteral("position"), position);
    data.insert(QStringLiteral("resource"), path);
    return EditorResult::success(data);
}
