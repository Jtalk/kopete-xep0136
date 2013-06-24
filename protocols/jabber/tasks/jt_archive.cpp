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

/// Static member initialization, XMPP NS field for archiving stanzas
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

void JT_Archive::requestPrefs()
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
#warning Collection manager is needed there
        //emit collectionListReceived(noIq);
        return true;
    } else if (isChat(noIq)) {
#warning And there
        //emit collectionItemReceived(noIq);
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
    QDomElement internalTag = e.firstChild().toElement();
    QString id = e.attribute("id");
    if (hasValidNS(e)) {
        // TODO: If we should do something on acknowledgement package receiving?
        return (this->*chooseHandler(e))(e, internalTag, id);
    } else return false;
}
