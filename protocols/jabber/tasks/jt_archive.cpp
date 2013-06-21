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

#include <QtCore/QDebug>

// Static member initialization, XMPP NS field for archiving stanzas
const QString JT_Archive::NS = "urn:xmpp:archive";

bool JT_Archive::hasValidNS(QDomElement e)
{
    bool found = false;
    QDomElement perf = findSubTag(e, "pref", &found);
    return found && perf.attribute("xmlns") == NS;
}

JT_Archive::JT_Archive(Task *const parent)
    : Task(parent)
{
}

void JT_Archive::onGo()
{
    // We must request our stored settings
    QDomElement request = uniformPrefsRequest();
    send(request);
}

QDomElement JT_Archive::uniformArchivingNS(const QString &tagName)
{
    QDomElement archiveNamespace = doc()->createElement(tagName);
    archiveNamespace.setAttribute("xmlns", NS);
    return archiveNamespace;
}

QDomElement JT_Archive::uniformPrefsRequest()
{
    // TODO: take care of the proper ID.
    QDomElement prefsRequest = createIQ(doc(), "get", "", "msgarch1");
    prefsRequest.appendChild( uniformArchivingNS("pref") );
    return prefsRequest;
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
    return false;
}

bool isList(const QDomElement &elem)
{
    return elem.tagName() == "list";
}

bool isChat(const QDomElement &elem)
{
    return elem.tagName() == "chat";
}

bool JT_Archive::handleResult(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    if (isPref(noIq)) {
        return m_preferences->writePrefs(noIq);
    } else if (isList(noIq)) {
#warning Collection manager is needed there
        //emit collectionListReceived(noIq);
    } else if (isChat(noIq)) {
#warning And there
        //emit collectionItemReceived(noIq);
    } else return false;
}

bool JT_Archive::handleError(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    return true;
}

bool isIq(const QDomElement &e)
{
    return e.tagName() == "iq";
}

/**
 * Converting text from XML to enumerations.
 */
JT_Archive::AnswerHandler JT_Archive::chooseHandler(QueryType type)
{
    switch (type) {
    case Set: return &JT_Archive::handleSet;
    case Get: return &JT_Archive::handleGet;
    case Result: return &JT_Archive::handleResult;
    case Error: return &JT_Archive::handleError;
    case Acknowledgement: return &JT_Archive::nothing;
    case NO_ARCHIVE: return &JT_Archive::nothing;
    }
    return &JT_Archive::nothing;
}
JT_Archive::QueryType queryType(const QDomElement &e)
{
    using namespace JT_Archive;
    // TODO: Fix this ugly mess somehow
    if (isIq(e)) {
        if (e.childNodes().isEmpty() && e.attribute("type") == "result") {
            return
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
    QueryType iqType = queryType(e);
    // TODO: If we should do something on acknowledgement package receiving?
    qDebug() << e.text();
    return iqType == Acknowledgement? true : this->*chooseHandler(iqType)(e, result, id);
}


