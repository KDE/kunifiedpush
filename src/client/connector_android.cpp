/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "connector_p.h"
#include "logging.h"

#include <QCoreApplication>
using QAndroidJniObject = QJniObject;

using namespace KUnifiedPush;

static QString fromJniString(JNIEnv *env, jstring s)
{
    if (!s) {
        return {};
    }

    const char *str = env->GetStringUTFChars(s, nullptr);
    const auto qs = QString::fromUtf8(str);
    env->ReleaseStringUTFChars(s, str);
    return qs;
}

static void newEndpoint(JNIEnv *env, jobject that, jstring token, jstring endpoint)
{
    Q_UNUSED(that);
    for (auto c : ConnectorPrivate::s_instances) {
        c->NewEndpoint(fromJniString(env, token), fromJniString(env, endpoint));
    }
}

static void registrationFailed(JNIEnv *env, jobject that, jstring token, jstring reason)
{
    Q_UNUSED(that);
    for (auto c :ConnectorPrivate::s_instances) {
        c->registrationFailed(fromJniString(env, token), fromJniString(env, reason));
    }
}

static void unregistered(JNIEnv *env, jobject that, jstring token)
{
    Q_UNUSED(that);
    for (auto c : ConnectorPrivate::s_instances) {
        c->Unregistered(fromJniString(env, token));
    }
}

static void message(JNIEnv *env, jobject that, jstring token, jbyteArray message, jstring messageId)
{
    Q_UNUSED(that);
    const auto messageSize = env->GetArrayLength(message);
    const auto messageData = env->GetByteArrayElements(message, nullptr);
    const auto messageQBA = QByteArray::fromRawData(reinterpret_cast<const char*>(messageData), messageSize);
    for (auto c : ConnectorPrivate::s_instances) {
        c->Message(fromJniString(env, token), messageQBA, fromJniString(env, messageId));
    }
    env->ReleaseByteArrayElements(message, messageData, JNI_ABORT);
}

static const JNINativeMethod methods[] = {
    {"newEndpoint", "(Ljava/lang/String;Ljava/lang/String;)V", (void*)newEndpoint},
    {"registrationFailed", "(Ljava/lang/String;Ljava/lang/String;)V", (void*)registrationFailed},
    {"unregistered", "(Ljava/lang/String;)V", (void*)unregistered},
    {"message", "(Ljava/lang/String;[BLjava/lang/String;)V", (void*)message},
};

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void*)
{
    static bool initialized = false;
    if (initialized) {
        return JNI_VERSION_1_4;
    }
    initialized = true;

    JNIEnv *env = nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
        qCWarning(Log) << "Failed to get JNI environment.";
        return -1;
    }
    jclass cls = env->FindClass("org/kde/kunifiedpush/MessageReceiver");
    if (env->RegisterNatives(cls, methods, sizeof(methods) / sizeof(JNINativeMethod)) < 0) {
        qCWarning(Log) << "Failed to register native functions.";
        return -1;
    }

    return JNI_VERSION_1_4;
}

std::vector<ConnectorPrivate*> ConnectorPrivate::s_instances;

void ConnectorPrivate::init()
{
    s_instances.push_back(this);
}

void ConnectorPrivate::deinit()
{
    s_instances.erase(std::remove(s_instances.begin(), s_instances.end(), this), s_instances.end());
}

void ConnectorPrivate::doSetDistributor(const QString &distServiceName)
{
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    m_distributor = QAndroidJniObject("org.kde.kunifiedpush.Distributor", "(Ljava/lang/String;Landroid/content/Context;)V", QAndroidJniObject::fromString(distServiceName).object(), context.object());
}

bool ConnectorPrivate::hasDistributor() const
{
    return m_distributor.isValid();
}

void ConnectorPrivate::doRegister()
{
    m_distributor.callMethod<void>("register", "(Ljava/lang/String;Ljava/lang/String;)V", QAndroidJniObject::fromString(m_token).object(), QAndroidJniObject::fromString(m_description).object());
}

void ConnectorPrivate::doUnregister()
{
    m_distributor.callMethod<void>("unregister", "(Ljava/lang/String;)V", QAndroidJniObject::fromString(m_token).object());
}
