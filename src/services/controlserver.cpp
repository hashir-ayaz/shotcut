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
        return m_editorService->addClipToTimeline(params.value(QStringLiteral("path")).toString(),
                                                trackIndex,
                                                position);
    }

    return EditorResult::failure(QStringLiteral("unknown method: %1").arg(method));
}
