/*
   Copyright (c) 2013 by Roman Nazarenko  <me@jtalk.me>

   Kopete    (c) 2008 by the Kopete developers <kopete-devel@kde.org>

   *************************************************************************
   *                                                                       *
   * This program is free software; you can redistribute it and/or modify  *
   * it under the terms of the GNU General Public License as published by  *
   * the Free Software Foundation; either version 2 of the License, or     *
   * (at your option) any later version.                                   *
   *                                                                       *
   *************************************************************************
*/

#include "jt_archive.h"

#include "xmpp_xmlcommon.h"
#include "xmpp_client.h"

const uint RSM_MAX = 30;

/// Static member initialization, XMPP NS field for archiving stanzas
const QString JT_Archive::ArchivingNS = "urn:xmpp:archive";
const QString JT_Archive::ResultSetManagementNS = "http://jabber.org/protocol/rsm";
//const QString JT_Archive::XMPP_UTF_DATETIME_FORMAT = "%Y.%m.%dT%H:%M:%SZ";
//const QString JT_Archive::XMPP_ZONE_DATETIME_FORMAT = "%Y.%m.%dT%H:%M:%S%:z";

bool JT_Archive::hasValidNS(const QDomElement &e)
{
    return e.attribute("xmlns") == ArchivingNS;
}

JT_Archive::JT_Archive(Task *const parent)
    : Task(parent)
{
    // TODO: Make residential mode for acknowledgements handling.
}

void JT_Archive::requestPrefs()
{
    // We must request our stored settings
    QDomElement request = uniformPrefsRequest();
    send(request);
}
#include <QDebug>
void JT_Archive::requestCollections(const CollectionsRequest &params)
{
    QDomElement requestIq = uniformCollectionsRequest(params);
    send(requestIq);
}

QDomElement JT_Archive::uniformUpdate(const QDomElement &tag)
{
    QDomElement prefsUpdate = createIQ(doc(), "set", "", client()->genUniqueId());
    QDomElement prefTag = uniformArchivingNS("pref");
    prefTag.appendChild(tag);
    prefsUpdate.appendChild(prefTag);
    return prefsUpdate;
}

QDomElement JT_Archive::uniformArchivingNS(const QString &tagName)
{
    //return doc()->createElementNS(ArchivingNS, tagName);
    QDomElement tag = doc()->createElement(tagName);
    tag.setAttribute("xmlns", ArchivingNS);
    return tag;

}

QDomElement JT_Archive::uniformPrefsRequest()
{
    // TODO: take care of the proper ID.
    QDomElement prefsRequest = createIQ(doc(), "get", "", client()->genUniqueId());
    prefsRequest.appendChild( uniformArchivingNS("pref") );
    return prefsRequest;
}

static inline bool isPref(const QDomElement &elem)
{
    return elem.tagName() == "pref";
}

bool JT_Archive::handleSet(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    Q_UNUSED(sessionID)
    Q_UNUSED(wholeElement)
    // Server pushes us new preferences, let's save them!
    // Server must send us 'set' requests only for new settings
    // pushing, so, if there's no <pref> tag inside <iq>, the
    // stanza is incorrect and should not be further processed.
    if (isPref(noIq)) {
        return writePrefs(noIq);
    } else {
        return false;
    }
}

bool JT_Archive::handleGet(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    Q_UNUSED(sessionID)
    Q_UNUSED(noIq)
    // That's weird. Server is not supposed to send GET IQs
    return false;
}

static inline bool isList(const QDomElement &elem)
{
    return elem.tagName() == "list";
}

static inline bool isChat(const QDomElement &elem)
{
    return elem.tagName() == "chat";
}

bool JT_Archive::handleResult(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    Q_UNUSED(sessionID)
    Q_UNUSED(wholeElement)
    if (isPref(noIq)) {
        return writePrefs(noIq);
    } else if (isList(noIq)) {
        qDebug() << "handleResult";
        return collectionsListReceived(noIq);
    } else if (isChat(noIq)) {
#warning And there
        //return chatReceived(noIq);
        return true;
    } else return false;
}

bool JT_Archive::handleError(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    Q_UNUSED(sessionID)
    Q_UNUSED(wholeElement)
    Q_UNUSED(noIq)
    return true;
}

static inline bool isIq(const QDomElement &e)
{
    return e.tagName() == "iq";
}

/**
 * Converting text from XML to enumerations.
 */
JT_Archive::AnswerHandler JT_Archive::chooseHandler(const QDomElement &e)
{
    // TODO: Fix this ugly mess somehow
    if (isIq(e)) {
        if (e.childNodes().isEmpty() && e.attribute("type") == "result") {
            return &JT_Archive::acknowledge;
        } else if (e.attribute("type") == "get") {
            return &JT_Archive::handleGet;
        } else if (e.attribute("type") == "set") {
            return &JT_Archive::handleSet;
        } else if (e.attribute("type") == "result") {
            qDebug() << "chooseHandler";
            return &JT_Archive::handleResult;
        } else if (e.attribute("type") == "error") {
            return &JT_Archive::handleError;
        }
    }
    return &JT_Archive::skip;
}

bool JT_Archive::take(const QDomElement &e)
{
    // TODO: Should we look through all tags instead?
    qDebug() << "take";
    QDomElement internalTag = e.firstChild().toElement();
    QString id = e.attribute("id");
    if (hasValidNS(internalTag)) {
        qDebug() << "hasValidNS";
        // TODO: If we should do something on acknowledgement package receiving?
        return (this->*chooseHandler(e))(e, internalTag, id);
    } else return false;
}

static QString DateTimeToXMPP(const KDateTime &dt)
{
    //return TS2stamp(dt.dateTime());
    return dt.toString(KDateTime::RFC3339Date);
}

static KDateTime XMPPtoDateTime(const QString &dt)
{
    //return KDateTime(stamp2TS(dt));
    return KDateTime::fromString(dt, KDateTime::RFC3339Date);
}


QDomElement JT_Archive::uniformCollectionsRequest(const CollectionsRequest &params)
{
    QDomElement listTag = uniformSkeletonCollectionsRequest(params.with);
    if (params.start.isValid()) {
        listTag.setAttribute("start", DateTimeToXMPP(params.start));
    }
    if (params.end.isValid()) {
        listTag.setAttribute("end", DateTimeToXMPP(params.end));
    }
    listTag.appendChild( uniformRsm(params.after) );
    QDomElement iq = createIQ(doc(), "get", "", client()->genUniqueId());
    iq.appendChild(listTag);
    return iq;
}

QDomElement JT_Archive::uniformRsmNS(const QString &tagName)
{
    //return doc()->createElementNS(ResultSetManagementNS, tagName);
    QDomElement tag = doc()->createElement(tagName);
    tag.setAttribute("xmlns", ResultSetManagementNS);
    return tag;
}

QDomElement JT_Archive::uniformRsmMax(uint max)
{
    QDomElement maxTag = doc()->createElement("max");
    QDomText internals = doc()->createTextNode(QString::number(max));
    maxTag.appendChild(internals);
    return maxTag;
}

QDomElement JT_Archive::uniformSkeletonCollectionsRequest(const QString &with)
{
    QDomElement skeleton = uniformArchivingNS("list");
    if (!with.isEmpty()) {
        skeleton.setAttribute("with", with);
    }
    return skeleton;
}

QDomElement JT_Archive::uniformRsm(const QString &after)
{
    QDomElement resultSetManagement = uniformRsmNS("set");
    resultSetManagement.appendChild( uniformRsmMax(RSM_MAX) );
    if (!after.isEmpty()) {
        QDomElement afterTag = doc()->createElement("after");
        afterTag.appendChild( doc()->createTextNode(after) );
        resultSetManagement.appendChild(afterTag);
    }
    return resultSetManagement;
}static inline QString capitalize(const QString &str)
{
    QString lowerStr = str.toLower();
    lowerStr[0] = lowerStr[0].toUpper();
    return lowerStr;
}

static inline bool verifyAutoTag(const QDomElement &autoTag)
{
    return autoTag.childNodes().isEmpty()
            && autoTag.attributes().contains("save");
}

static inline bool hasScope(const QDomElement &autoTag)
{
    return autoTag.attributes().contains("scope");
}

#define STR(x) # x
#define STRCAT(x,y) QString(QString(x) + "_" + QString(y))
#define EXTRACT_TAG(type, tag, name) (type)s_ ## type ## Enum.keyToValue(STRCAT(STR(type) \
    , tag.attribute(name)).toAscii())

bool JT_Archive::handleAutoTag(const QDomElement &autoTag)
{
    // This will return either received scope or static defaultScope variable,
    // if no correct scope is received;
    AutoScope scope = EXTRACT_TAG(AutoScope, autoTag, "scope");
    if (scope == -1) {
        scope = defaultScope;
    }
    // <auto> Must be an empty tag and must contain 'save' attribute
    // according to the current standard draft.
    // If there was no scope, 'scope' variable would be defaultScope, which is
    // valid.
    if (verifyAutoTag(autoTag)) {
        bool isAutoArchivingEnabled = QVariant(autoTag.attribute("save")).toBool();
        emit automaticArchivingEnable(isAutoArchivingEnabled, scope);
        return true;
    } else {
        return false;
    }
}

static inline bool verifyDefaultTag(const QDomElement &defaultTag)
{
    return defaultTag.childNodes().isEmpty()
            && defaultTag.attributes().contains("save")
            && defaultTag.attributes().contains("otr");
}

bool JT_Archive::handleDefaultTag(const QDomElement &defaultTag)
{
    DefaultSave saveMode = EXTRACT_TAG(DefaultSave, defaultTag, "save");
    DefaultOtr otrMode = EXTRACT_TAG(DefaultOtr, defaultTag, "otr");
    // <default> Must be an empty tag and must contain 'save' and 'otr' attributes
    // according to the current standard draft.
    if (verifyDefaultTag(defaultTag) && saveMode != -1 && otrMode != -1) {
        uint expire = defaultTag.attributes().contains("expire")
                ? QVariant(defaultTag.attribute("scope")).toUInt()
                : defaultExpiration;
        emit defaultPreferenceChanged(saveMode, otrMode, expire);
        return true;
    } else {
        return false;
    }
}

bool JT_Archive::handleItemTag(const QDomElement &)
{
    // TODO: Implement per-user storing settings
    return true;
}

bool JT_Archive::handleSessionTag(const QDomElement &)
{
    // TODO: Implement per-session storing settings
    return true;
}

static inline bool verifyMethodTag(const QDomElement &tag) {
    return tag.childNodes().isEmpty()
            && tag.attributes().contains("type")
            && tag.attributes().contains("use");
}

bool JT_Archive::handleMethodTag(const QDomElement &methodTag)
{
    MethodType method = EXTRACT_TAG(MethodType, methodTag, "type");
    MethodUse use = EXTRACT_TAG(MethodUse, methodTag, "use");
    if (verifyMethodTag(methodTag) && method != -1 && use != -1) {
        emit archivingMethodChanged(method, use);
        return true;
    } else return false;
}

static QDomElement findRSMTag(const QDomElement &tag, bool *found)
{
    QDomElement rsmTag = tag.tagName() == "set" ? tag : findSubTag(tag, "set", 0);
    *found = rsmTag.attribute("xmlns") == JT_Archive::ResultSetManagementNS;
    return rsmTag;
}

JT_Archive::RSMInfo JT_Archive::parseRSM(const QDomElement &elem)
{
    bool found = false;
    QDomElement rsmTag = findRSMTag(elem, &found);
    if (found) {
        RSMInfo info;
        info.count = findSubTag(rsmTag, "count", 0).text().toUInt();
        info.first = findSubTag(rsmTag, "first", 0).text();
        info.last = findSubTag(rsmTag, "last", 0).text();
        return info;
    } else {
        return RSMInfo();
    }
}

#define DOM_FOREACH(var, domElement) for(QDomNode var = domElement.firstChild(); !var.isNull(); var = var.nextSibling())

QList<JT_Archive::ChatInfo> JT_Archive::parseChatInfo(const QDomElement &tags)
{
    QList<ChatInfo> list;
    DOM_FOREACH(chatTag, tags) {
        QDomElement currentElem = chatTag.toElement();
        if (currentElem.tagName() == "chat") {
            ChatInfo info;
            info.with = currentElem.attribute("with");
            info.time = XMPPtoDateTime( currentElem.attribute("start") );
            list.append(info);
        } else continue;
    }
    return list;
}

bool JT_Archive::writePrefs(const QDomElement &tags)
{
    bool isSuccessful = true;
    DOM_FOREACH(tag, tags) {
        bool isCurrentSuccessful = writePref(tag.toElement());
        isSuccessful = isSuccessful && isCurrentSuccessful;
    }
    return isSuccessful;
}

bool JT_Archive::writePref(const QDomElement &elem)
{
    TagNames tagName = (TagNames)s_TagNamesEnum.keyToValue(capitalize(elem.tagName()).toAscii());
    switch (tagName) {
    case Auto: return handleAutoTag(elem);
    case Default: return handleDefaultTag(elem);
    case Item: return handleItemTag(elem);
    case Session: return handleSessionTag(elem);
    case Method: return handleMethodTag(elem);
    default: return false;
    }
}

bool JT_Archive::collectionsListReceived(const QDomElement &listTag)
{
    RSMInfo rsmInfo = parseRSM(listTag);
    QList<ChatInfo> list = parseChatInfo(listTag);
    emit collectionsReceived(list, rsmInfo);
    return true;
}

#define VALUE(type, value) QString( s_ ## type ## Enum.valueToKey(value) ).remove(QLatin1String(# type) + "_")
#define INSERT_TAG(type, tag, name, value) tag.setAttribute(name, VALUE(type, value))

void JT_Archive::updateDefault(const JT_Archive::DefaultSave save,
                               const JT_Archive::DefaultOtr otr,
                               const uint expiration)
{
    Q_UNUSED(expiration)
    QDomElement tag = doc()->createElement("default");
    INSERT_TAG(DefaultSave, tag, "save", save);
    INSERT_TAG(DefaultOtr, tag, "otr", otr);
    QDomElement update = uniformUpdate(tag);
    send(update);
}

void JT_Archive::updateAuto(bool isEnabled, const JT_Archive::AutoScope scope)
{
    QDomElement tag = doc()->createElement("auto");
    tag.setAttribute("save", QVariant(isEnabled).toString());
    if (scope != -1) {
        INSERT_TAG(AutoScope, tag, "scope", scope);
    }
    QDomElement update = uniformUpdate(tag);
    send(update);
}

void JT_Archive::updateStorage(const JT_Archive::MethodType method, const JT_Archive::MethodUse use)
{
    QDomElement tag = doc()->createElement("method");
    INSERT_TAG(MethodType, tag, "type", method);
    INSERT_TAG(MethodUse, tag, "use", use);
    QDomElement update = uniformUpdate(tag);
    send(update);
}

