/*
 * Copyright (c) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef CONTROLSERVER_H
#define CONTROLSERVER_H

#include "editortypes.h"

#include <QObject>

class EditorService;
class QWebSocket;
class QWebSocketServer;

class ControlServer : public QObject
{
    Q_OBJECT

public:
    explicit ControlServer(EditorService *editorService, QObject *parent = nullptr);
    ~ControlServer();

    bool start(quint16 port);

private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString &message);
    QString processMessage(const QString &message);

private:
    EditorResult dispatchMethod(const QString &method, const QJsonObject &params);

    EditorService *m_editorService;
    QWebSocketServer *m_server{nullptr};
    QList<QWebSocket *> m_clients;
};

#endif // CONTROLSERVER_H
