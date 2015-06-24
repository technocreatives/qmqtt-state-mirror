/*
 * The MIT License (MIT)
 *
 * Copyright (c) <2015> <Niklas Frisk>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#ifndef STATEMANAGER_H
#define STATEMANAGER_H

#include <QObject>
#include <QHash>
#include <QSignalMapper>
#include <QMetaMethod>
#include "qmqtt.h"
#include "mqttstatemirror_global.h"

class MQTTSTATEMIRRORSHARED_EXPORT StateManager : public QObject
{
    Q_OBJECT

    QHash<QString, QObject*> m_topicMapping;
    QHash<QObject*, QString> m_objectMapping;
    QHash<QObject*, qint64> m_revisionMapping;
    QSignalMapper m_signalMapper;
    QMetaMethod m_signalMapperMapSlot;

    QMQTT::Client* m_mqttClient;

    bool m_isConnected;

public:
    explicit StateManager(QObject *parent = 0, QMQTT::Client* mqttClient = 0);

signals:

public slots:

    bool registerObject(QObject* object, QString topic);
    bool unregisterObject(QObject* object);
    bool unregisterTopic(QString topic);
    void mapped(QObject* object);

    //MQTT slots...
    void connected();
    void disconnected();
    void published(QMQTT::Message &message);
    void received(const QMQTT::Message &message);
    void subscribed(const QString &topic);
    void unsubscribed(const QString &topic);
};

#endif // STATEMANAGER_H
