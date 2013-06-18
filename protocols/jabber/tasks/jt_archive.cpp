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
    case NO_ARCHIVE: break;
    }
}

#define DOM_FOREACH(var, domElement) for(QDomNode var = domElement.firstChild(); !var.isNull(); var = var.nextSibling())

void JT_Archive::handleSet(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    // Server pushes us the new preferences, let's save them!
    DOM_FOREACH(node, noIq) {
        QDomElement elem = node.toElement();
        if (elem.isNull()) {
            continue;
        }

        writePref(elem);
    }
}

void JT_Archive::handleGet(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    qDebug() << "That's weird. Server is not supposed to send GET IQs, received stanza is " << wholeElement.text() << endl;
}

void JT_Archive::handleResult(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    // Server just sent us something we requested

}

void JT_Archive::handleError(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
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


QString JT_Archive::Preferences::toString(JT_Archive::Preferences::Scope sc)
{
#error NOIMPLEMENT
}

QString JT_Archive::Preferences::toString(JT_Archive::Preferences::Save save)
{
#error NOIMPLEMENT
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
