/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "editorservice.h"

#include "commands/filtercommands.h"
#include "commands/subtitlecommands.h"
#include "commands/timelinecommands.h"
#include "controllers/filtercontroller.h"
#include "docks/subtitlesdock.h"
#include "docks/timelinedock.h"
#include "jobqueue.h"
#include "jobs/abstractjob.h"
#include "jobs/meltjob.h"
#include "jobs/postjobaction.h"
#include "jobs/whisperjob.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/attachedfiltersmodel.h"
#include "models/multitrackmodel.h"
#include "models/subtitlesmodel.h"
#include "qmltypes/qmlfilter.h"
#include "qmltypes/qmlmetadata.h"
#include "services/transitionpresets.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

#include <MltAnimation.h>
#include <MltFilter.h>
#include <MltTransition.h>
#include <MltTractor.h>

#include <QFileInfo>
#include <QJsonArray>
#include <QLocale>
#include <QStandardPaths>
#include <QTemporaryFile>

namespace {

Mlt::Transition *findLumaTransition(Mlt::Producer &producer)
{
    QScopedPointer<Mlt::Service> service(producer.producer());
    while (service && service->is_valid()) {
        if (service->type() == mlt_service_transition_type) {
            Mlt::Transition transition(*service);
            const QString mltService = QString::fromUtf8(transition.get("mlt_service"));
            if (mltService == QStringLiteral("luma")
                || mltService == QStringLiteral("movit.luma_mix"))
                return new Mlt::Transition(transition);
        }
        service.reset(service->producer());
    }
    return nullptr;
}

void applyPresetToProducer(Mlt::Producer &producer, const TransitionPreset &preset)
{
    QScopedPointer<Mlt::Transition> transition(findLumaTransition(producer));
    if (!transition || !transition->is_valid())
        return;

    if (preset.resource.isEmpty()) {
        transition->set("resource", "");
        transition->set("softness", preset.softness);
    } else {
        transition->set("resource", preset.resource.toUtf8().constData());
        transition->set("softness", preset.softness);
        transition->set("progressive", preset.progressive ? 1 : 0);
    }
    MLT.refreshConsumer();
}

QJsonObject transitionJsonFromClip(MultitrackModel *model,
                                   TimelineDock *timelineDock,
                                   int trackIndex,
                                   int clipIndex)
{
    QJsonObject clipObject;
    const QModelIndex clipModelIndex = model->makeIndex(trackIndex, clipIndex);
    clipObject.insert(QStringLiteral("index"), clipIndex);
    clipObject.insert(QStringLiteral("name"),
                      model->data(clipModelIndex, MultitrackModel::NameRole).toString());
    clipObject.insert(QStringLiteral("duration"),
                      model->data(clipModelIndex, MultitrackModel::DurationRole).toInt());
    clipObject.insert(QStringLiteral("isTransition"),
                      model->data(clipModelIndex, MultitrackModel::IsTransitionRole).toBool());

    if (!clipObject.value(QStringLiteral("isTransition")).toBool())
        return clipObject;

    Mlt::Producer producer = timelineDock->producerForClip(trackIndex, clipIndex);
    if (producer.is_valid()) {
        QScopedPointer<Mlt::Transition> luma(findLumaTransition(producer));
        if (luma && luma->is_valid()) {
            const QString resource = QString::fromUtf8(luma->get("resource"));
            clipObject.insert(QStringLiteral("lumaResource"), resource);
            clipObject.insert(QStringLiteral("softness"), luma->get_double("softness"));
            const TransitionPreset *preset = nullptr;
            for (const TransitionPreset &candidate : TransitionPresets::all()) {
                if (candidate.resource == resource
                    || (resource.isEmpty() && candidate.id == QStringLiteral("dissolve"))
                    || (!candidate.resource.isEmpty()
                        && resource.contains(candidate.resource.mid(1)))) {
                    preset = &candidate;
                    break;
                }
            }
            if (preset)
                clipObject.insert(QStringLiteral("transitionPreset"), preset->id);
        }
    }
    return clipObject;
}

Mlt::Filter *findShotcutFilter(Mlt::Producer *producer, const QString &filterId)
{
    if (!producer || !producer->is_valid())
        return nullptr;
    for (int i = 0; i < producer->filter_count(); ++i) {
        Mlt::Filter *filter = producer->filter(i);
        if (filter && filter->is_valid()
            && filterId == QString::fromUtf8(filter->get(kShotcutFilterProperty))) {
            return filter;
        }
        delete filter;
    }
    return nullptr;
}

Mlt::Filter *ensureShotcutFilter(Mlt::Producer *producer, const QString &filterId)
{
    if (Mlt::Filter *existing = findShotcutFilter(producer, filterId))
        return existing;

    QmlMetadata *metadata = MAIN.filterController()->metadata(filterId);
    if (!metadata)
        return nullptr;

    Mlt::Filter filter(MLT.profile(), metadata->mlt_service().toUtf8().constData());
    filter.set(kShotcutFilterProperty, filterId.toUtf8().constData());
    producer->attach(filter);
    return findShotcutFilter(producer, filterId);
}

mlt_keyframe_type interpolationFromString(const QString &interpolation)
{
    if (interpolation == QStringLiteral("hold") || interpolation == QStringLiteral("discrete"))
        return mlt_keyframe_discrete;
    if (interpolation == QStringLiteral("smooth"))
        return mlt_keyframe_smooth;
    return mlt_keyframe_linear;
}

QString propertyParamName(const QString &propertyId)
{
    if (propertyId == QStringLiteral("rotation"))
        return QStringLiteral("transition.fix_rotate_x");
    if (propertyId == QStringLiteral("opacity"))
        return QStringLiteral("alpha");
    return QStringLiteral("transition.rect");
}

QString filterIdForProperty(const QString &propertyId)
{
    if (propertyId == QStringLiteral("opacity"))
        return QStringLiteral("brightnessOpacity");
    return QStringLiteral("affineSizePosition");
}

EditorResult failureIfInvalidClip(MultitrackModel *model, int trackIndex, int clipIndex)
{
    if (trackIndex < 0 || trackIndex >= model->trackList().size())
        return EditorResult::failure(QStringLiteral("invalid trackIndex"));
    if (clipIndex < 0 || clipIndex >= MAIN.timelineDock()->clipCount(trackIndex))
        return EditorResult::failure(QStringLiteral("invalid clipIndex"));
    return EditorResult::success();
}

} // namespace

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

    SubtitlesModel *subtitlesModel = timelineDock->subtitlesModel();
    if (subtitlesModel)
        data.insert(QStringLiteral("subtitleTrackCount"), subtitlesModel->trackCount());

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
                QJsonObject clipObject = transitionJsonFromClip(model, timelineDock, trackIndex, clipIndex);
                const QModelIndex clipModelIndex = model->makeIndex(trackIndex, clipIndex);
                clipObject.insert(QStringLiteral("resource"),
                                  model->data(clipModelIndex, MultitrackModel::ResourceRole)
                                      .toString());
                clipObject.insert(QStringLiteral("start"),
                                  model->data(clipModelIndex, MultitrackModel::StartRole).toInt());
                clipObject.insert(QStringLiteral("inPoint"),
                                  model->data(clipModelIndex, MultitrackModel::InPointRole).toInt());
                clipObject.insert(QStringLiteral("outPoint"),
                                  model->data(clipModelIndex, MultitrackModel::OutPointRole).toInt());
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
                                              int position,
                                              int inPoint,
                                              int outPoint,
                                              bool append)
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
    if (!append && position < 0)
        position = timelineDock->position();

    if (trackIndex < 0 || trackIndex >= model->trackList().size())
        return EditorResult::failure(QStringLiteral("invalid trackIndex: %1").arg(trackIndex));

    Mlt::Producer producer(MLT.profile(), path.toUtf8().constData());
    if (!producer.is_valid())
        return EditorResult::failure(QStringLiteral("failed to load media: %1").arg(path));

    Mlt::Producer *readyProducer = MLT.setupNewProducer(&producer);
    if (!readyProducer || !readyProducer->is_valid())
        return EditorResult::failure(QStringLiteral("failed to prepare media: %1").arg(path));

    // Optional source trim so incoming clips can accept AddTransitionByTrimIn
    // (requires frame_in >= transition duration).
    const int maxOut = qMax(0, readyProducer->get_length() - 1);
    int resolvedIn = inPoint < 0 ? 0 : inPoint;
    int resolvedOut = outPoint < 0 ? maxOut : outPoint;
    if (resolvedIn > maxOut)
        resolvedIn = maxOut;
    if (resolvedOut > maxOut)
        resolvedOut = maxOut;
    if (resolvedOut < resolvedIn)
        return EditorResult::failure(QStringLiteral("outPoint must be >= inPoint"));
    readyProducer->set_in_and_out(resolvedIn, resolvedOut);

    const QString xml = MLT.XML(readyProducer);
    if (append) {
        m_mainWindow->undoStack()->push(new Timeline::AppendCommand(*model, trackIndex, xml));
    } else {
        m_mainWindow->undoStack()->push(
            new Timeline::OverwriteCommand(*model, trackIndex, position, xml));
    }

    QJsonObject data;
    data.insert(QStringLiteral("trackIndex"), trackIndex);
    data.insert(QStringLiteral("position"), append ? -1 : position);
    data.insert(QStringLiteral("inPoint"), resolvedIn);
    data.insert(QStringLiteral("outPoint"), resolvedOut);
    data.insert(QStringLiteral("append"), append);
    data.insert(QStringLiteral("resource"), path);
    return EditorResult::success(data);
}

EditorResult EditorService::listTransitionPresets()
{
    QJsonArray presets;
    for (const TransitionPreset &preset : TransitionPresets::all())
        presets.append(TransitionPresets::toJson(preset));
    QJsonObject data;
    data.insert(QStringLiteral("presets"), presets);
    return EditorResult::success(data);
}

EditorResult EditorService::applyTransition(int trackIndex,
                                           int clipIndex,
                                           const QString &presetId,
                                           int durationFrames)
{
    const TransitionPreset *preset = TransitionPresets::find(presetId);
    if (!preset)
        return EditorResult::failure(QStringLiteral("unknown preset: %1").arg(presetId));
    if (durationFrames <= 0)
        return EditorResult::failure(QStringLiteral("durationFrames must be positive"));

    TimelineDock *timelineDock = m_mainWindow->timelineDock();
    MultitrackModel *model = timelineDock->model();
    if (!m_mainWindow->isMultitrackValid())
        return EditorResult::failure(QStringLiteral("timeline is not valid"));

    if (clipIndex <= 0)
        return EditorResult::failure(QStringLiteral("clipIndex must be incoming clip at a join"));

    const int duration = -durationFrames;
    if (!model->addTransitionByTrimInValid(trackIndex, clipIndex, duration))
        return EditorResult::failure(QStringLiteral("cannot apply transition at this join"));

    m_mainWindow->undoStack()->push(new Timeline::AddTransitionByTrimInCommand(*timelineDock,
                                                                               trackIndex,
                                                                               clipIndex,
                                                                               duration,
                                                                               0));

    const int transitionClipIndex = clipIndex;
    Mlt::Producer producer = timelineDock->producerForClip(trackIndex, transitionClipIndex);
    if (producer.is_valid())
        applyPresetToProducer(producer, *preset);

    QJsonObject data;
    data.insert(QStringLiteral("trackIndex"), trackIndex);
    data.insert(QStringLiteral("transitionClipIndex"), transitionClipIndex);
    data.insert(QStringLiteral("presetId"), presetId);
    data.insert(QStringLiteral("durationFrames"), durationFrames);
    return EditorResult::success(data);
}

EditorResult EditorService::setTransitionDuration(int trackIndex,
                                                  int transitionClipIndex,
                                                  int durationFrames)
{
    if (durationFrames <= 0)
        return EditorResult::failure(QStringLiteral("durationFrames must be positive"));

    MultitrackModel *model = m_mainWindow->timelineDock()->model();
    const QModelIndex clipModelIndex = model->makeIndex(trackIndex, transitionClipIndex);
    if (!model->data(clipModelIndex, MultitrackModel::IsTransitionRole).toBool())
        return EditorResult::failure(QStringLiteral("clip is not a transition"));

    const int currentDuration = model->data(clipModelIndex, MultitrackModel::DurationRole).toInt();
    const int delta = durationFrames - currentDuration;
    const int handleDelta = -delta / 2;
    if (!model->resizeTransitionValid(trackIndex, transitionClipIndex, handleDelta))
        return EditorResult::failure(QStringLiteral("invalid transition duration"));

    m_mainWindow->undoStack()->push(new Timeline::ResizeTransitionCommand(*model,
                                                                          trackIndex,
                                                                          transitionClipIndex,
                                                                          handleDelta));

    QJsonObject data;
    data.insert(QStringLiteral("trackIndex"), trackIndex);
    data.insert(QStringLiteral("transitionClipIndex"), transitionClipIndex);
    data.insert(QStringLiteral("durationFrames"), durationFrames);
    return EditorResult::success(data);
}

EditorResult EditorService::getTransition(int trackIndex, int transitionClipIndex)
{
    MultitrackModel *model = m_mainWindow->timelineDock()->model();
    const QModelIndex clipModelIndex = model->makeIndex(trackIndex, transitionClipIndex);
    if (!model->data(clipModelIndex, MultitrackModel::IsTransitionRole).toBool())
        return EditorResult::failure(QStringLiteral("clip is not a transition"));

    QJsonObject data = transitionJsonFromClip(model,
                                              m_mainWindow->timelineDock(),
                                              trackIndex,
                                              transitionClipIndex);
    return EditorResult::success(data);
}

EditorResult EditorService::removeTransition(int trackIndex, int transitionClipIndex)
{
    MultitrackModel *model = m_mainWindow->timelineDock()->model();
    const QModelIndex clipModelIndex = model->makeIndex(trackIndex, transitionClipIndex);
    if (!model->data(clipModelIndex, MultitrackModel::IsTransitionRole).toBool())
        return EditorResult::failure(QStringLiteral("clip is not a transition"));

    model->removeTransition(trackIndex, transitionClipIndex);

    QJsonObject data;
    data.insert(QStringLiteral("trackIndex"), trackIndex);
    data.insert(QStringLiteral("transitionClipIndex"), transitionClipIndex);
    return EditorResult::success(data);
}

EditorResult EditorService::listAnimatableProperties(int trackIndex, int clipIndex)
{
    MultitrackModel *model = m_mainWindow->timelineDock()->model();
    const EditorResult valid = failureIfInvalidClip(model, trackIndex, clipIndex);
    if (!valid.ok)
        return valid;

    QJsonArray properties;
    auto addProperty = [&](const QString &id, const QString &label, const QString &filterId) {
        QJsonObject property;
        property.insert(QStringLiteral("propertyId"), id);
        property.insert(QStringLiteral("label"), label);
        property.insert(QStringLiteral("filterId"), filterId);
        properties.append(property);
    };
    addProperty(QStringLiteral("position.x"), QStringLiteral("Position X"), QStringLiteral("affineSizePosition"));
    addProperty(QStringLiteral("position.y"), QStringLiteral("Position Y"), QStringLiteral("affineSizePosition"));
    addProperty(QStringLiteral("scale"), QStringLiteral("Scale"), QStringLiteral("affineSizePosition"));
    addProperty(QStringLiteral("rotation"), QStringLiteral("Rotation"), QStringLiteral("affineSizePosition"));
    addProperty(QStringLiteral("opacity"), QStringLiteral("Opacity"), QStringLiteral("brightnessOpacity"));

    QJsonObject data;
    data.insert(QStringLiteral("properties"), properties);
    return EditorResult::success(data);
}

EditorResult EditorService::addKeyframe(int trackIndex,
                                        int clipIndex,
                                        const QString &propertyId,
                                        int frame,
                                        double value,
                                        const QString &interpolation)
{
    MultitrackModel *model = m_mainWindow->timelineDock()->model();
    const EditorResult valid = failureIfInvalidClip(model, trackIndex, clipIndex);
    if (!valid.ok)
        return valid;

    Mlt::Producer producer = m_mainWindow->timelineDock()->producerForClip(trackIndex, clipIndex);
    if (!producer.is_valid())
        return EditorResult::failure(QStringLiteral("invalid clip producer"));

    const QString filterId = filterIdForProperty(propertyId);
    Mlt::Filter *filter = ensureShotcutFilter(&producer, filterId);
    if (!filter)
        return EditorResult::failure(QStringLiteral("failed to ensure filter"));

    QmlMetadata *metadata = MAIN.filterController()->metadata(filterId);
    QmlFilter qmlFilter(*filter, metadata);
    const mlt_keyframe_type keyframeType = interpolationFromString(interpolation);

    if (propertyId == QStringLiteral("opacity")) {
        qmlFilter.set(QStringLiteral("alpha"), value, frame, keyframeType);
    } else if (propertyId == QStringLiteral("rotation")) {
        qmlFilter.set(QStringLiteral("transition.fix_rotate_x"), value, frame, keyframeType);
    } else if (propertyId == QStringLiteral("scale")) {
        QRectF rect = qmlFilter.getRect(QStringLiteral("transition.rect"));
        if (rect.isNull()) {
            rect = QRectF(0, 0, MLT.profile().width(), MLT.profile().height());
        }
        const double centerX = rect.x() + rect.width() / 2.0;
        const double centerY = rect.y() + rect.height() / 2.0;
        const double width = MLT.profile().width() * value;
        const double height = MLT.profile().height() * value;
        qmlFilter.set(QStringLiteral("transition.rect"),
                      centerX - width / 2.0,
                      centerY - height / 2.0,
                      width,
                      height,
                      1.0,
                      frame,
                      keyframeType);
    } else if (propertyId == QStringLiteral("position.x")
               || propertyId == QStringLiteral("position.y")) {
        QRectF rect = qmlFilter.getRect(QStringLiteral("transition.rect"));
        if (rect.isNull()) {
            rect = QRectF(0, 0, MLT.profile().width(), MLT.profile().height());
        }
        if (propertyId == QStringLiteral("position.x"))
            rect.moveLeft(value);
        else
            rect.moveTop(value);
        qmlFilter.set(QStringLiteral("transition.rect"), rect, frame, keyframeType);
    } else {
        delete filter;
        return EditorResult::failure(QStringLiteral("unknown propertyId"));
    }

    delete filter;
    MLT.refreshConsumer();

    QJsonObject data;
    data.insert(QStringLiteral("propertyId"), propertyId);
    data.insert(QStringLiteral("frame"), frame);
    data.insert(QStringLiteral("value"), value);
    return EditorResult::success(data);
}

EditorResult EditorService::removeKeyframe(int trackIndex,
                                           int clipIndex,
                                           const QString &propertyId,
                                           int frame)
{
    MultitrackModel *model = m_mainWindow->timelineDock()->model();
    const EditorResult valid = failureIfInvalidClip(model, trackIndex, clipIndex);
    if (!valid.ok)
        return valid;

    Mlt::Producer producer = m_mainWindow->timelineDock()->producerForClip(trackIndex, clipIndex);
    if (!producer.is_valid())
        return EditorResult::failure(QStringLiteral("invalid clip producer"));

    const QString filterId = filterIdForProperty(propertyId);
    Mlt::Filter *filter = findShotcutFilter(&producer, filterId);
    if (!filter)
        return EditorResult::failure(QStringLiteral("filter not found"));

    QmlMetadata *metadata = MAIN.filterController()->metadata(filterId);
    QmlFilter qmlFilter(*filter, metadata);
    const QString param = propertyParamName(propertyId);
    Mlt::Animation animation = qmlFilter.getAnimation(param);
    if (!animation.is_valid()) {
        delete filter;
        return EditorResult::failure(QStringLiteral("animation not found"));
    }
    for (int i = 0; i < animation.key_count(); ++i) {
        if (animation.key_get_frame(i) == frame) {
            animation.remove(frame);
            animation.interpolate();
            break;
        }
    }
    delete filter;
    MLT.refreshConsumer();

    QJsonObject data;
    data.insert(QStringLiteral("propertyId"), propertyId);
    data.insert(QStringLiteral("frame"), frame);
    return EditorResult::success(data);
}

EditorResult EditorService::listKeyframes(int trackIndex,
                                          int clipIndex,
                                          const QString &propertyId)
{
    MultitrackModel *model = m_mainWindow->timelineDock()->model();
    const EditorResult valid = failureIfInvalidClip(model, trackIndex, clipIndex);
    if (!valid.ok)
        return valid;

    Mlt::Producer producer = m_mainWindow->timelineDock()->producerForClip(trackIndex, clipIndex);
    if (!producer.is_valid())
        return EditorResult::failure(QStringLiteral("invalid clip producer"));

    const QString filterId = filterIdForProperty(propertyId);
    Mlt::Filter *filter = findShotcutFilter(&producer, filterId);
    if (!filter)
        return EditorResult::failure(QStringLiteral("filter not found"));

    QmlMetadata *metadata = MAIN.filterController()->metadata(filterId);
    QmlFilter qmlFilter(*filter, metadata);
    const QString param = propertyParamName(propertyId);
    Mlt::Animation animation = qmlFilter.getAnimation(param);

    QJsonArray keyframes;
    if (animation.is_valid()) {
        for (int i = 0; i < animation.key_count(); ++i) {
            const int keyFrame = animation.key_get_frame(i);
            QJsonObject keyframe;
            keyframe.insert(QStringLiteral("frame"), keyFrame);
            keyframe.insert(QStringLiteral("value"), qmlFilter.getDouble(param, keyFrame));
            keyframes.append(keyframe);
        }
    }
    delete filter;

    QJsonObject data;
    data.insert(QStringLiteral("propertyId"), propertyId);
    data.insert(QStringLiteral("keyframes"), keyframes);
    return EditorResult::success(data);
}

EditorResult EditorService::setPropertyValue(int trackIndex,
                                             int clipIndex,
                                             const QString &propertyId,
                                             double value)
{
    return addKeyframe(trackIndex, clipIndex, propertyId, -1, value, QStringLiteral("linear"));
}

EditorResult EditorService::transcribeCaptions(const QString &language,
                                               bool translateToEnglish,
                                               const QString &trackName)
{
    if (!m_mainWindow->isMultitrackValid())
        return EditorResult::failure(QStringLiteral("timeline is not valid"));

    if (Settings.whisperExe().isEmpty() || Settings.whisperModel().isEmpty())
        return EditorResult::failure(
            QStringLiteral("Whisper is not configured; set whisper executable and model in Settings"));

    SubtitlesDock *subtitlesDock = m_mainWindow->subtitlesDock();
    if (!subtitlesDock)
        return EditorResult::failure(QStringLiteral("subtitles dock unavailable"));

    QString name = trackName;
    if (name.isEmpty())
        name = QStringLiteral("Auto Captions");

    const QString xml = MLT.XML(m_mainWindow->multitrack());
    std::unique_ptr<Mlt::Producer> tempProducer(
        new Mlt::Producer(MLT.profile(), "xml-string", xml.toUtf8().constData()));
    Mlt::Tractor tractor(*tempProducer);
    if (!tractor.is_valid())
        return EditorResult::failure(QStringLiteral("invalid timeline tractor"));

    const QString tmpLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                + "/";
    QTemporaryFile *tmpWav = Util::writableTemporaryFile(tmpLocation, "shotcut-XXXXXX.wav");
    if (!tmpWav->open()) {
        delete tmpWav;
        return EditorResult::failure(QStringLiteral("failed to create temporary wav file"));
    }
    tmpWav->close();

    QStringList args;
    args << "-consumer" << "avformat:" + tmpWav->fileName() << "video_off=1" << "ar=16000"
         << "ac=1";
    const QString wavJobName = QObject::tr("Extracting Audio");
    MeltJob *wavJob = new MeltJob(wavJobName,
                                  MLT.XML(tempProducer.get()),
                                  args,
                                  MLT.profile().frame_rate_num(),
                                  MLT.profile().frame_rate_den());
    tmpWav->setParent(wavJob);
    JOBS.add(wavJob);

    QTemporaryFile *tmpSrt = Util::writableTemporaryFile(tmpLocation, "shotcut-XXXXXX.srt");
    if (!tmpSrt->open()) {
        delete tmpSrt;
        return EditorResult::failure(QStringLiteral("failed to create temporary srt file"));
    }
    tmpSrt->close();

    const QString whisperJobName = QObject::tr("Speech to Text");
    WhisperJob *whisperJob = new WhisperJob(whisperJobName,
                                            tmpWav->fileName(),
                                            tmpSrt->fileName(),
                                            language,
                                            translateToEnglish,
                                            42,
                                            Settings.whisperUseGpu());
    QString langCode = language;
    const QLocale::Language lang = QLocale::codeToLanguage(langCode);
    if (lang != QLocale::AnyLanguage)
        langCode = QLocale::languageToCode(lang, QLocale::ISO639Part2);

    whisperJob->setPostJobAction(new ImportSrtPostJobAction(tmpSrt->fileName(),
                                                            name,
                                                            langCode,
                                                            false,
                                                            subtitlesDock));
    tmpSrt->setParent(whisperJob);
    JOBS.add(whisperJob);
    m_lastJobLabel = whisperJobName;

    QJsonObject data;
    data.insert(QStringLiteral("jobId"), whisperJobName);
    data.insert(QStringLiteral("trackName"), name);
    return EditorResult::success(data);
}

EditorResult EditorService::getJobStatus(const QString &jobId)
{
    const QString lookupId = jobId.isEmpty() ? m_lastJobLabel : jobId;
    if (lookupId.isEmpty())
        return EditorResult::failure(QStringLiteral("jobId is required"));

    for (AbstractJob *job : JOBS.jobs()) {
        if (job->label() != lookupId)
            continue;
        QJsonObject data;
        data.insert(QStringLiteral("jobId"), job->label());
        data.insert(QStringLiteral("finished"), job->isFinished());
        data.insert(QStringLiteral("running"), job->state() == QProcess::Running);
        return EditorResult::success(data);
    }
    return EditorResult::failure(QStringLiteral("job not found: %1").arg(lookupId));
}

EditorResult EditorService::getCaptions()
{
    SubtitlesModel *model = m_mainWindow->timelineDock()->subtitlesModel();
    if (!model || !model->isValid())
        return EditorResult::failure(QStringLiteral("no subtitles loaded"));

    QJsonArray tracks;
    for (int trackIndex = 0; trackIndex < model->trackCount(); ++trackIndex) {
        QJsonObject trackObject;
        const SubtitlesModel::SubtitleTrack track = model->getTrack(trackIndex);
        trackObject.insert(QStringLiteral("index"), trackIndex);
        trackObject.insert(QStringLiteral("name"), track.name);
        trackObject.insert(QStringLiteral("language"), track.lang);

        QJsonArray cues;
        for (int cueIndex = 0; cueIndex < model->itemCount(trackIndex); ++cueIndex) {
            const Subtitles::SubtitleItem &item = model->getItem(trackIndex, cueIndex);
            QJsonObject cue;
            cue.insert(QStringLiteral("index"), cueIndex);
            cue.insert(QStringLiteral("startMs"), static_cast<qint64>(item.start));
            cue.insert(QStringLiteral("endMs"), static_cast<qint64>(item.end));
            cue.insert(QStringLiteral("text"), QString::fromStdString(item.text));
            cues.append(cue);
        }
        trackObject.insert(QStringLiteral("cues"), cues);
        tracks.append(trackObject);
    }

    QJsonObject data;
    data.insert(QStringLiteral("tracks"), tracks);
    return EditorResult::success(data);
}

EditorResult EditorService::setCaptionText(int trackIndex, int cueIndex, const QString &text)
{
    SubtitlesModel *model = m_mainWindow->timelineDock()->subtitlesModel();
    if (!model || !model->isValid())
        return EditorResult::failure(QStringLiteral("no subtitles loaded"));
    if (trackIndex < 0 || trackIndex >= model->trackCount())
        return EditorResult::failure(QStringLiteral("invalid trackIndex"));
    if (cueIndex < 0 || cueIndex >= model->itemCount(trackIndex))
        return EditorResult::failure(QStringLiteral("invalid cueIndex"));

    m_mainWindow->undoStack()->push(
        new Subtitles::SetTextCommand(*model, trackIndex, cueIndex, text));

    QJsonObject data;
    data.insert(QStringLiteral("trackIndex"), trackIndex);
    data.insert(QStringLiteral("cueIndex"), cueIndex);
    data.insert(QStringLiteral("text"), text);
    return EditorResult::success(data);
}

EditorResult EditorService::importCaptionsFile(const QString &srtPath,
                                               const QString &trackName)
{
    if (srtPath.isEmpty())
        return EditorResult::failure(QStringLiteral("path is required"));
    if (!QFileInfo::exists(srtPath))
        return EditorResult::failure(QStringLiteral("file does not exist: %1").arg(srtPath));

    SubtitlesDock *subtitlesDock = m_mainWindow->subtitlesDock();
    if (!subtitlesDock)
        return EditorResult::failure(QStringLiteral("subtitles dock unavailable"));

    QString name = trackName;
    if (name.isEmpty())
        name = QStringLiteral("Imported Captions");

    subtitlesDock->importSrtFromFile(srtPath, name, QStringLiteral("eng"), false);

    QJsonObject data;
    data.insert(QStringLiteral("path"), srtPath);
    data.insert(QStringLiteral("trackName"), name);
    return EditorResult::success(data);
}
