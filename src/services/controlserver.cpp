/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "controlserver.h"

#include "Logger.h"
#include "editorservice.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QWebSocket>
#include <QWebSocketServer>

ControlServer::ControlServer(EditorService *editorService, QObject *parent)
    : QObject(parent)
    , m_editorService(editorService)
{
}

ControlServer::~ControlServer()
{
    for (QWebSocket *client : m_clients) {
        client->close();
        client->deleteLater();
    }
    m_clients.clear();
    delete m_server;
    m_server = nullptr;
}

bool ControlServer::start(quint16 port)
{
    if (m_server)
        return true;

    m_server = new QWebSocketServer(QStringLiteral("ShotcutControlServer"),
                                    QWebSocketServer::NonSecureMode,
                                    this);
    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        LOG_ERROR() << "ControlServer failed to listen on port" << port << m_server->errorString();
        delete m_server;
        m_server = nullptr;
        return false;
    }

    connect(m_server, &QWebSocketServer::newConnection, this, &ControlServer::onNewConnection);
    LOG_INFO() << "ControlServer listening on ws://127.0.0.1:" << port;
    return true;
}

void ControlServer::onNewConnection()
{
    QWebSocket *client = m_server->nextPendingConnection();
    if (!client)
        return;

    m_clients.append(client);
    connect(client, &QWebSocket::textMessageReceived, this, &ControlServer::onTextMessageReceived);
    connect(client, &QWebSocket::disconnected, this, [this, client]() {
        m_clients.removeAll(client);
        client->deleteLater();
    });
}

void ControlServer::onTextMessageReceived(const QString &message)
{
    QWebSocket *client = qobject_cast<QWebSocket *>(sender());
    if (!client)
        return;

    QString response;
    if (thread() == m_editorService->thread()) {
        response = processMessage(message);
    } else {
        QMetaObject::invokeMethod(this,
                                  "processMessage",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QString, response),
                                  Q_ARG(QString, message));
    }
    client->sendTextMessage(response);
}

QString ControlServer::processMessage(const QString &message)
{
    QJsonParseError parseError;
    const QJsonDocument requestDocument = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !requestDocument.isObject()) {
        QJsonObject errorResponse;
        errorResponse.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
        errorResponse.insert(QStringLiteral("id"), QJsonValue::Null);
        QJsonObject errorObject;
        errorObject.insert(QStringLiteral("code"), -32700);
        errorObject.insert(QStringLiteral("message"), QStringLiteral("Parse error"));
        errorResponse.insert(QStringLiteral("error"), errorObject);
        return QString::fromUtf8(QJsonDocument(errorResponse).toJson(QJsonDocument::Compact));
    }

    const QJsonObject request = requestDocument.object();
    const QJsonValue idValue = request.value(QStringLiteral("id"));
    const QString method = request.value(QStringLiteral("method")).toString();
    const QJsonObject params = request.value(QStringLiteral("params")).toObject();

    QJsonObject response;
    response.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    response.insert(QStringLiteral("id"), idValue);

    if (method.isEmpty()) {
        QJsonObject errorObject;
        errorObject.insert(QStringLiteral("code"), -32600);
        errorObject.insert(QStringLiteral("message"), QStringLiteral("Invalid Request"));
        response.insert(QStringLiteral("error"), errorObject);
        return QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact));
    }

    const EditorResult result = dispatchMethod(method, params);
    response.insert(QStringLiteral("result"), result.toJsonObject());
    return QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact));
}

EditorResult ControlServer::dispatchMethod(const QString &method, const QJsonObject &params)
{
    if (method == QStringLiteral("ping"))
        return m_editorService->ping();

    if (method == QStringLiteral("get_state"))
        return m_editorService->getState();

    if (method == QStringLiteral("open_project"))
        return m_editorService->openProject(params.value(QStringLiteral("path")).toString());

    if (method == QStringLiteral("save_project"))
        return m_editorService->saveProject(params.value(QStringLiteral("path")).toString());

    if (method == QStringLiteral("add_clip")) {
        int trackIndex = params.value(QStringLiteral("trackIndex")).toInt(-1);
        int position = params.value(QStringLiteral("position")).toInt(-1);
        int inPoint = params.value(QStringLiteral("inPoint")).toInt(-1);
        int outPoint = params.value(QStringLiteral("outPoint")).toInt(-1);
        bool append = params.value(QStringLiteral("append")).toBool(false);
        return m_editorService->addClipToTimeline(params.value(QStringLiteral("path")).toString(),
                                                trackIndex,
                                                position,
                                                inPoint,
                                                outPoint,
                                                append);
    }

    if (method == QStringLiteral("list_transition_presets"))
        return m_editorService->listTransitionPresets();

    if (method == QStringLiteral("apply_transition")) {
        return m_editorService->applyTransition(params.value(QStringLiteral("trackIndex")).toInt(),
                                                params.value(QStringLiteral("clipIndex")).toInt(),
                                                params.value(QStringLiteral("presetId")).toString(),
                                                params.value(QStringLiteral("durationFrames"))
                                                    .toInt());
    }

    if (method == QStringLiteral("set_transition_duration")) {
        return m_editorService->setTransitionDuration(
            params.value(QStringLiteral("trackIndex")).toInt(),
            params.value(QStringLiteral("transitionClipIndex")).toInt(),
            params.value(QStringLiteral("durationFrames")).toInt());
    }

    if (method == QStringLiteral("get_transition")) {
        return m_editorService->getTransition(params.value(QStringLiteral("trackIndex")).toInt(),
                                              params.value(QStringLiteral("transitionClipIndex"))
                                                  .toInt());
    }

    if (method == QStringLiteral("remove_transition")) {
        return m_editorService->removeTransition(params.value(QStringLiteral("trackIndex")).toInt(),
                                                 params.value(QStringLiteral("transitionClipIndex"))
                                                     .toInt());
    }

    if (method == QStringLiteral("list_animatable_properties")) {
        return m_editorService->listAnimatableProperties(
            params.value(QStringLiteral("trackIndex")).toInt(),
            params.value(QStringLiteral("clipIndex")).toInt());
    }

    if (method == QStringLiteral("add_keyframe")) {
        return m_editorService->addKeyframe(params.value(QStringLiteral("trackIndex")).toInt(),
                                            params.value(QStringLiteral("clipIndex")).toInt(),
                                            params.value(QStringLiteral("propertyId")).toString(),
                                            params.value(QStringLiteral("frame")).toInt(),
                                            params.value(QStringLiteral("value")).toDouble(),
                                            params.value(QStringLiteral("interpolation"))
                                                .toString(QStringLiteral("linear")));
    }

    if (method == QStringLiteral("remove_keyframe")) {
        return m_editorService->removeKeyframe(params.value(QStringLiteral("trackIndex")).toInt(),
                                               params.value(QStringLiteral("clipIndex")).toInt(),
                                               params.value(QStringLiteral("propertyId")).toString(),
                                               params.value(QStringLiteral("frame")).toInt());
    }

    if (method == QStringLiteral("list_keyframes")) {
        return m_editorService->listKeyframes(params.value(QStringLiteral("trackIndex")).toInt(),
                                              params.value(QStringLiteral("clipIndex")).toInt(),
                                              params.value(QStringLiteral("propertyId")).toString());
    }

    if (method == QStringLiteral("set_clip_property")) {
        return m_editorService->setPropertyValue(params.value(QStringLiteral("trackIndex")).toInt(),
                                                 params.value(QStringLiteral("clipIndex")).toInt(),
                                                 params.value(QStringLiteral("propertyId")).toString(),
                                                 params.value(QStringLiteral("value")).toDouble());
    }

    if (method == QStringLiteral("transcribe_captions")) {
        return m_editorService->transcribeCaptions(
            params.value(QStringLiteral("language")).toString(QStringLiteral("en")),
            params.value(QStringLiteral("translateToEnglish")).toBool(),
            params.value(QStringLiteral("trackName")).toString());
    }

    if (method == QStringLiteral("get_job_status"))
        return m_editorService->getJobStatus(params.value(QStringLiteral("jobId")).toString());

    if (method == QStringLiteral("get_captions"))
        return m_editorService->getCaptions();

    if (method == QStringLiteral("set_caption_text")) {
        return m_editorService->setCaptionText(params.value(QStringLiteral("trackIndex")).toInt(),
                                               params.value(QStringLiteral("cueIndex")).toInt(),
                                               params.value(QStringLiteral("text")).toString());
    }

    if (method == QStringLiteral("import_captions")) {
        return m_editorService->importCaptionsFile(params.value(QStringLiteral("path")).toString(),
                                                   params.value(QStringLiteral("trackName"))
                                                       .toString());
    }

    return EditorResult::failure(QStringLiteral("unknown method: %1").arg(method));
}
