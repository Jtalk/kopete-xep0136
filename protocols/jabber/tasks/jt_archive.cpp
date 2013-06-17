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

bool JT_Archive::parsePerfs(const QDomElement &e)
{
    for(QDomNode n = e.firstChild(); !n.isNull(); n = n.nextSibling()) {
        QDomElement current = n.toElement();

        if(current.isNull()) {
            continue;
        }

        if(current.tagName() == name) {
            if(found)
                *found = true;
            return i;
        }
    }
}


