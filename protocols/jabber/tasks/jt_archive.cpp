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

/// Let's have some fun! Using macro concatenation and Qt's metasystem to convert string
/// representation of the enum value
#define getEnumData(enumName, keyName, method) static_cast<enumName>( m_ ## enumName ## Enum. ## method ## ( keyName ));
#define getEnumKey(enumName, keyName) getEnumData(enumName, keyName, keyToValue)
#define getEnumValue(enumName, keyName) getEnumData(enumName, keyName, valueToKey)

/// We need capitalized properties names for QMetaEnum
inline QString capitalize(QString str)
{
    str = str.toLower();
    str[0] = str[0].toUpper();
    return str;
}

/// Since enums in C++ are extremly dumb and the language itself gives us no way to check
/// their casting properly, we need this nail.
#define validateEnum(enumName, value) m_ ## enumName ## Enum.valueToKey(value) != -1
#define setEnumValue(enumName, value) if (validateEnum(enumName,value)) { m_ ## enumName ## = value; } \
    else { qDebug() << "In " << __FILE__ << ":" << __LINE__ << " error: wrong enum, " << (int)value; }


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
    }
}


void JT_Archive::initMetaEnums()
{
    QMetaObject metaObject = JT_Archive::staticMetaObject;
    const JT_Archive::m_ScopeEnum = metaObject.enumerator( metaObject.indexOfEnumerator( "Scope" ) );
    const JT_Archive::m_SaveModeEnum = metaObject.enumerator( metaObject.indexOfEnumerator( "Save" ) );
    const JT_Archive::m_OTRModeEnum = metaObject.enumerator( metaObject.indexOfEnumerator( "Otr" ) );
    const JT_Archive::m_ArchiveMethodEnum = metaObject.enumerator( metaObject.indexOfEnumerator( "Type" ) );
    const JT_Archive::m_ArchivePriorityEnum = metaObject.enumerator( metaObject.indexOfEnumerator( "Use" ) );
}

void JT_Archive::setScope(JT_Archive::Scope sc)
{
    setEnumValue(Scope, sc);
}
void JT_Archive::setScope(const QString &s)
{
    setScope( getEnumKey(Scope, s) );
}

Scope JT_Archive::scope()
{
    return m_Scope;

}
QString JT_Archive::scope()
{
    return getEnumValue(Scope, m_Scope);
}

void JT_Archive::setSaveMode(JT_Archive::Save sm)
{
    setEnumValue(Save, sm);
}

void JT_Archive::setSaveMode(const QString &s)
{
    setSaveMode( getEnumKey(Save, s) );
}

JT_Archive::Save JT_Archive::saveMode()
{
    return m_Save;
}

QString JT_Archive::saveMode()
{
    return getEnumValue(Save, m_Save)
}

void JT_Archive::setOTRMode(JT_Archive::Otr otrm)
{
    setEnumValue(Otr, sm);
}

void JT_Archive::setOTRMode(const QString &s)
{
    setOTRMode( getEnumKey(Otr, s) );
}

JT_Archive::Otr JT_Archive::otrMode()
{
    return m_Otr;
}

QString JT_Archive::otrMode()
{
    return getEnumValue(Otr, m_Otr);
}

void JT_Archive::setArchiveStorage(JT_Archive::Type am, JT_Archive::Use ap)
{
    if (validateEnum(Type, am) && validateEnum(Use, ap)) {
        m_StorageSetting[am] = ap;
    } else {
        qDebug() << "In " << __FILE__ << ":" << __LINE__ << " error: wrong enums, " << (int)am << "\t" << (int)ap;
    }
}

void JT_Archive::setArchiveStorage(const QString &am, const QString &ap)
{
    JT_Archive::Type am_enum = getEnumKey(Type, am);
    JT_Archive::Use ap_enum = getEnumKey(Use, ap);
    setArchiveStorage(am_enum, ap_enum);
}

JT_Archive::Use JT_Archive::archiveStorage(JT_Archive::Type am)
{
    return m_StorageSetting[am];
}

QString JT_Archive::archiveStorage(const QString &am)
{
    JT_Archive::Type am_enum = getEnumKey(Type, am);
    JT_Archive::Use use_enum = archiveStorage(am_enum);
    return getEnumData(Use, use_enum);
}

