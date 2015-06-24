/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Niklas Frisk
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

#include "statemanager.h"
#include <QMetaProperty>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include <QJSValue>

StateManager::StateManager(QObject *parent, QMQTT::Client* mqttClient): QObject(parent), m_mqttClient(mqttClient), m_isConnected(false)
{
    QObject::connect(&m_signalMapper, SIGNAL(mapped(QObject*)), this, SLOT(mapped(QObject*)));

    int index = m_signalMapper.metaObject()->indexOfSlot(QMetaObject::normalizedSignature("map()"));

    if(index > -1) {
        m_signalMapperMapSlot = m_signalMapper.metaObject()->method(index);
        qDebug() << "derp";
    }


    if(m_mqttClient) {
        connect(m_mqttClient, SIGNAL(connected()),this, SLOT(connected()));
        connect(m_mqttClient, SIGNAL(published(QMQTT::Message&)),this, SLOT(published(QMQTT::Message&)));
        connect(m_mqttClient, SIGNAL(received(const QMQTT::Message)),this, SLOT(received(const QMQTT::Message)));
        connect(m_mqttClient, SIGNAL(subscribed(const QString)),this, SLOT(subscribed(const QString)));
        connect(m_mqttClient, SIGNAL(unsubscribed(const QString)),this, SLOT(unsubscribed(const QString)));
        connect(m_mqttClient, SIGNAL(disconnected()),this, SLOT(disconnected()));

        m_isConnected = m_mqttClient->isConnected();
    }
}


bool StateManager::registerObject(QObject* object, QString topic)
{
    if(m_objectMapping.find(object) != m_objectMapping.end() || m_topicMapping.find(topic) != m_topicMapping.end()) {
        return false;
    }

    //Connect all notification signals on the properties defined on the object.
    const QMetaObject* metaObject = object->metaObject();
    for(int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i) {
       QMetaProperty metaProperty = metaObject->property(i);
       if(metaProperty.hasNotifySignal()){
           QObject::connect(object, metaProperty.notifySignal(), &m_signalMapper, m_signalMapperMapSlot);
       }
    }

    m_signalMapper.setMapping(object, object);

    m_revisionMapping.insert(object, 0);
    m_objectMapping.insert(object, topic);
    m_topicMapping.insert(topic, object);

    if(m_isConnected) {
        m_mqttClient->subscribe(topic, 0);
    }

    return true;
}

bool StateManager::unregisterObject(QObject* object)
{
    auto objectIterator = m_objectMapping.find(object);

    if(objectIterator != m_objectMapping.end())
    {
        //Disconnect all notifications.
        const QMetaObject* metaObject = object->metaObject();
        for(int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i) {
           QMetaProperty metaProperty = metaObject->property(i);
           if(metaProperty.hasNotifySignal()){
               QObject::disconnect(object, metaProperty.notifySignal(), &m_signalMapper, m_signalMapperMapSlot);
           }
        }

        if(m_isConnected) {
            m_mqttClient->unsubscribe(objectIterator.value());
        }

        m_topicMapping.remove(objectIterator.value());
        m_objectMapping.remove(objectIterator.key());
        m_revisionMapping.remove(objectIterator.key());

        return true;
    }

    return false;
}

bool StateManager::unregisterTopic(QString topic)
{
    auto topicIterator = m_topicMapping.find(topic);

    if(topicIterator != m_topicMapping.end())
    {
        return this->unregisterObject(topicIterator.value());
    }
    return false;
}


void StateManager::mapped(QObject *object) {
    auto objectIterator = m_objectMapping.find(object);

    if(m_isConnected && objectIterator != m_objectMapping.end())
    {
        qint64 rev = m_revisionMapping.find(object).value() + 1;
        m_revisionMapping.insert(object, rev);


        QJsonObject serialized;

        // Get all properties that this object has defined
        const QMetaObject* metaObject = object->metaObject();
        QList<QString> properties;

        //Gather static properties
        for(int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i) {
           QMetaProperty metaProperty = metaObject->property(i);
           properties << metaProperty.name();
        }

        //Gather dynamic properties
        QList<QByteArray> dynamicProperties = object->dynamicPropertyNames();
        auto i = dynamicProperties.begin();
        while(i != dynamicProperties.end()) {
            properties.append(QString(*i++));
        }

        auto j = properties.begin();

        //Serialize all property values gathered in previous step.
        while(j != properties.end())
        {
            QVariant var = object->property((*j).toUtf8());

            if (var.userType() == qMetaTypeId<QJSValue>()) {
                var = var.value<QJSValue>().toVariant();
            }

            serialized.insert(*j, QJsonValue::fromVariant(var));
            ++j;
        }


        QJsonObject payload;
        payload.insert(QString("body"), serialized);
        payload.insert(QString("revision"), QJsonValue::fromVariant(QVariant(rev)));
        quint16 mid = 0;
        quint8 qos = 0;
        bool retain = false;
        bool dup = false;

        QMQTT::Message message;
        message.setTopic(objectIterator.value());
        message.setPayload(QJsonDocument(payload).toJson());

        message.setId(mid);
        message.setQos(qos);
        message.setRetain(retain);
        message.setDup(dup);

        m_mqttClient->publish(message);
    }
}

//MQTT slots...
void StateManager::connected()
{
    m_isConnected = true;

    auto i = m_topicMapping.begin();

    while(i != m_topicMapping.end()) {
        m_mqttClient->subscribe((i++).key(), 0);
    }
}
void StateManager::disconnected()
{
    m_isConnected = false;
}
void StateManager::received(const QMQTT::Message &message)
{
    auto topicIterator = m_topicMapping.find(message.topic());

    if(topicIterator != m_topicMapping.end()) {
        QObject* object = topicIterator.value();
        qint64 localRevision = m_revisionMapping.find(object).value();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(message.payload());

        if(!jsonResponse.isNull()) {
            QJsonObject jsonObject = jsonResponse.object();

            if(!jsonObject.contains("body") || !jsonObject.contains("revision")) {
                qDebug() << "Got bad data for topic " << message.topic() << ", ignoring.";
                return;
            }

            QJsonValue jsonRev = jsonObject.value("revision");
            if(!jsonRev.isDouble()) {
                qDebug() << "Got bad revision (" << jsonRev.toString() << ") on topic " << message.topic() << ", ignoring.";
                return;
            }

            qint64 messageRevision = jsonRev.toVariant().toLongLong();
            if(localRevision >= messageRevision) {
                qDebug() << "Got message with lower revision (" << messageRevision << ") than local (" << localRevision << "), ignoring.";
                return;
            }

            QJsonObject body = jsonObject.value("body").toObject();
            QStringList keys = body.keys();

            //Block change signals to avoid cyclic publishing (this should be fine if all updates to models are on the same thread).
            m_signalMapper.blockSignals(true);

            foreach (QString key, keys) {
                QVariant value = object->property(key.toUtf8().constData());

                // If key dosen't exist amongst object properties continue to next.
                if (!value.isValid()) {
                    continue;
                }

                QJsonValue jsonValue = body.value(key);
                object->setProperty(key.toUtf8().constData(), jsonValue.toVariant());
            }

            //Reenable signals
            m_revisionMapping.insert(object, messageRevision);
            m_signalMapper.blockSignals(false);
        } else {
            qDebug() << "Got bad data for topic " << message.topic() << ", ignoring.";
        }
    }
}

void StateManager::published(QMQTT::Message &message) {}
void StateManager::subscribed(const QString &topic) {}
void StateManager::unsubscribed(const QString &topic) {}
