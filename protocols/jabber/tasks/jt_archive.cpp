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

bool JT_Archive::hasValidNS(QDomElement e)
{
    bool found = false;
    QDomElement perf = findSubTag(e, "pref", &found);
    return found && perf.attribute("xmlns") == NS;
}

JT_Archive::JT_Archive(const Task *parent)
    : Task(parent)
{
    // Static member initialization, XMPP NS field for archiving stanzas
    const QString JT_Archive::NS = "urn:xmpp:archive";

    setScope();
    setSaveMode();
    setOTRMode();

    // We are setting auto-archiving as forbidden by default because server-side
    // storing is a security breakdown if server location is not protected against
    // unauthorized access. User must explicitly allow server-side archiving.
    setArchiveStorage(Auto, Forbid);
    setArchiveStorage(Local, Prefer);
    setArchiveStorage(Manual, Concede);

}

QDomElement JT_Archive::uniformArchivingNS()
{
    QDomElement archiveNamespace = doc()->createElement("pref");
    perfsRequestBody.setAttribute("xmlns", NS);
    return archiveNamespace;
}

QDomElement JT_Archive::uniformPrefsRequest()
{
    // TODO: take care of the proper ID.
    QDomElement prefsRequest = createIQ(doc(), "get", "", "msgarch1");
    prefsRequest.appendChild( uniformArchivingNS() );
    return perfsRequest;
}

QDomElement JT_Archive::uniformPrefsSetting()
{
}

JT_Archive::AnswerHandler JT_Archive::chooseHandler(Preferences::QueryType type)
{
    using namespace JT_Archive::Preferences;
    switch (type) {
    case Set: return &handleSet;
    case Get: return &handleGet;
    case Result: return &handleResult;
    case Error: return &handleError;
    case Acknowledgement: return &nothing;
    case NO_ARCHIVE: return &nothing;
    }
}

bool JT_Archive::Preferences::handleAutoTag(const QDomElement &elem)
{
    // <auto> Must be an empty tag and must contain 'save' attribute
    // according to the current standard draft
    if (elem.childNodes().isEmpty() && elem.attributes().contains("save")) {
        m_auto_save = QVariant(elem.attribute("save")).toBool();

        if (!elem.attribute("scope").isEmpty()) {
            m_auto_scope = toScope(elem.attribute("scope"));
        }
        return true;
    } else {
        return false;
    }
}

bool JT_Archive::Preferences::handleDefaultTag(const QDomElement &elem)
{
    // <default> Must be an empty tag and must contain 'save' and 'otr' attributes
    // according to the current standard draft
    if (elem.childNodes().isEmpty()
            && elem.attributes().contains("save")
            && elem.attributes().contains("otr")) {
        m_default_save = toSave(elem.attribute("save"));
        m_default_otr = toOtr(elem.attribute("otr"));

        if (elem.attributes().contains("expire")) {
            m_default_expire = QVariant(elem.attribute("scope")).toUInt();
        }
        return true;
    } else {
        return false;
    }
}

bool JT_Archive::Preferences::handleItemTag(const QDomElement &)
{
#error NOIMPLEMENT
}

bool JT_Archive::Preferences::handleSessionTag(const QDomElement &)
{
#error NOIMPLEMENT
}

bool JT_Archive::Preferences::handleMethodTag(const QDomElement &elem)
{
    if (elem.childNodes().isEmpty()
            && elem.attributes().contains("type")
            && elem.attributes().contains("use")) {
        QString type = elem.attribute("type");
        QString use = elem.attribute("use");
        m_method.insert(toType(type), toUse(use));
    } else {
        return false;
    }
}

#define DOM_FOREACH(var, domElement) for(QDomNode var = domElement.firstChild(); !var.isNull(); var = var.nextSibling())

bool JT_Archive::Preferences::writePrefs(const QDomElement &tags)
{
    bool isSuccessful = true;
    DOM_FOREACH(tag, tags) {
        isSuccessful = isSuccessful && writePref(tag.toElement());
    }
    return isSuccessful;
}

bool JT_Archive::Preferences::writePref(const QDomElement &elem)
{
    QString tagName = elem.tagName();
    if (tagName == "auto") {
        return handleAutoTag(elem);
    } else if (tagName == "default") {
        return handleDefaultTag(elem);
    } else if (tagName == "item") {
        return handleItemTag(elem);
    } else if (tagName == "session") {
        return handleSessionTag(elem);
    } else if (tagName == "method") {
        return handleMethodTag(elem);
    } else {
        // Unknown tag?
        qDebug() << "Unknown tag: " << elem.text() << endl;
        return false;
    }
}

bool isPref(const QDomElement &elem)
{
    return elem.tagName() == "pref";
}

bool JT_Archive::handleSet(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    // Server pushes us new preferences, let's save them!
    // Server must send us 'set' requests only for new settings
    // pushing, so, if there's no <pref> tag inside <iq>, the
    // stanza is incorrect and should not be further processed.
    if (isPref(noIq)) {
        return m_preferences->writePrefs(noIq);
    } else {
        return false;
    }

}

bool JT_Archive::handleGet(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    qDebug() << "That's weird. Server is not supposed to send GET IQs, received stanza is " << wholeElement.text() << endl;
}

bool isList(const QDomElement &elem)
{
    return elem.tagName() == "list";
}

bool JT_Archive::handleResult(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    if (isPref(noIq)) {
        return m_preferences->writePrefs(noIq);
    } else if (isList(noIq)) {
        emit collectionListReceived(noIq);
    } else if (isChat(noIq)) {
        emit collectionItemReceived(noIq);
    }
}

bool JT_Archive::handleError(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
}

bool JT_Archive::parsePerfs(const QDomElement &e)
{
    for(QDomNode n = e.firstChild(); !n.isNull(); n = n.nextSibling()) {
        QDomElement current = n.toElement();

        if(current.tagName() == name) {
            if(found)
                *found = true;
            return i;
        }
    }
}

bool isIq(const QDomElement &e)
{
    return e.tagName() == "iq";
}

/**
 * Converting text from XML to enumerations.
 */
JT_Archive::Preferences::QueryType queryType(const QDomElement &e)
{
    using namespace JT_Archive::Preferences;
    // TODO: Fix this ugly mess somehow
    if (isIq(e)) {
        if (e.childNodes().isEmpty() && e.attribute("type") == "result") {
            return Acknowledgement;
        } else if (e.attribute("type") == "get") {
            return Get;
        } else if (e.attribute("type") == "set") {
            return Set;
        } else if (e.attribute("type") == "result") {
            return Result;
        } else if (e.attribute("type") == "error") {
            return Error;
        }
    }
    return NO_ARCHIVE;
}

bool JT_Archive::take(const QDomElement &e)
{
    Preferences::QueryType iqType = queryType(e);
    // TODO: If we should do something on acknowledgement package receiving?
    return iqType == Preferences::Acknowledgement? true : this->*chooseHandler(iqType)(e, result, id);
}
