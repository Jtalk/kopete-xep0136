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

/**
 * This is an XMPP extension protocol 0136 implementation (Message Archiving)
 * task for Kopete.
 * Look at http://xmpp.org/extensions/xep-0136.html for further details.
 */

#ifndef JT_ARCHIVE_H
#define JT_ARCHIVE_H

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QPair>

#include <QtCore/QMetaObject>
#include <QtCore/QMetaEnum>

#include "xmpp_task.h"
#include "xmpp_jid.h"
#include "xmpp_xdata.h"

class QDomElement;
using XMPP::Task;

class JT_Archive : public Task
{
    Q_OBJECT
    Q_ENUMS( Scope )
    Q_ENUMS( Save )
    Q_ENUMS( Otr )
    Q_ENUMS( Type )
    Q_ENUMS( Use )
public:
    static const QString NS; // urn:xmpp:archive
    static bool hasValidNS(QDomElement);

    /// Scope of archiving setting: forever or for the current stream only
    enum Scope { Global, Stream };

    /// Which part of the messaging stream should we store?
    enum Save {
        Body,       // Only <body> text
        False,      // Nothing
        Message,    // The whole <message> XML
        Stream      // The whole stream, byte-to-byte
    };

    /// How do we proceed Off-the-Record?
    enum Otr {
        Approve,    // the user MUST explicitly approve off-the-record communication.
        Concede,    // communications MAY be off the record if requested by another user.
        Forbid,     // communications MUST NOT be off the record.
        Oppose,     // communications SHOULD NOT be off the record even if requested.
        Prefer,     // communications SHOULD be off the record if possible.
        Require    // communications MUST be off the record.
    };

    /// Where and how hard does user prefer to store archive?
    enum Type {
        Auto,   // Server-side automatic archiving
        Local,  // Archiving to the local file/database
        Manual  // Manual server-side archiving
    };
    enum Use {
        Concede,    // Use if no other methods are available
        Forbid,     // Never use this method
        Prefer     // Use if available
    };

    JT_Archive(const Task *parent);

    void go(bool autoDelete = false);
    virtual bool take(const QDomElement &);

    strToPref(const QString&);

    // Setters and getters.
    // I skip any enum validation, because if one wish to shoot his leg, no one
    // can ever stop him from doing so.
    void setScope(Scope sc = Stream);
    void setScope(const QString &s);
    Scope scope();
    QString scope();

    void setSaveMode(Save sm = Message);
    void setSaveMode(const QString &s);
    Save saveMode();
    QString saveMode();

    void setOTRMode(Otr otrm = Approve);
    void setOTRMode(const QString &s);
    Otr otrMode();
    QString otrMode();

    void setArchiveStorage(Type am, Use ap);
    void setArchiveStorage(const QString& am, const QString& ap);
    Use archiveStorage(Type am);
    QString archiveStorage(const QString& am);

protected:
    /**
     * \brief uniformArchivingNS creates DOM element <pref xmlns=NS></pref>.
     * This function uses static NS element of the archiving class.
     * Result can be used to embed any XEP-0136 data.
     */
    QDomElement uniformArchivingNS();

    QDomElement uniformPrefsRequest();

    QDomElement uniformPrefsSetting();

    bool parsePerfs(const QDomElement &);
private:
    /**
     * \brief initMetaEnums extracts enumerations information from the Qt meta system.
     * This will initialize static QMetaEnum members for further use.
     */
    void initMetaEnums();

    Scope m_Scope;
    static const QMetaEnum m_ScopeEnum;

    Save m_Save;
    static const QMetaEnum m_SaveEnum;

    Otr m_Otr;
    static const QMetaEnum m_OtrEnum;

    QMap<Type, Use> m_StorageSetting;
    static const QMetaEnum m_TypeEnum;
    static const QMetaEnum m_UseEnum;
};

#endif JT_ARCHIVE_H
