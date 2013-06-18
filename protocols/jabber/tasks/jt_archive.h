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
public:
    static const QString NS; // urn:xmpp:archive
    static bool hasValidNS(QDomElement);

    /**
     * \brief The Preferences struct contains XEP-0136 preferences representation,
     * as well as string-to-enum and vice versa conversion routines.
     */
    struct Preferences {
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

        enum QueryType {
            Get,            // <iq type='get'>
            Set,            // <iq type=... etc.
            Result,
            Acknowledgement, // Empty iq. Server uses them to show us an acknowlegement of changes previously sent to it.
            Error,
            NO_ARCHIVE      // This stanza is not supposed to be handled by an archiving task
        };

        static QString toString(Scope);
        static QString toString(Save);
        static QString toString(Otr);
        static QString toString(Type);
        static QString toString(Use);

        static Scope toScope(const QString&);
        static Save toSave(const QString&);
        static Otr toOtr(const QString&);
        static Type toType(const QString&);
        static Use toUse(const QString&);
    };



    JT_Archive(const Task *parent);

    void go(bool autoDelete = false);
    virtual bool take(const QDomElement &);

    strToPref(const QString&);

    // Setters and getters.
    // I skip any enum validation, because if one wish to shoot his leg, no one
    // can ever stop him from doing so.
    void setScope(Scope sc = Stream);
    Scope scope();

    void setSaveMode(Save sm = Message);
    Save saveMode();

    void setOTRMode(Otr otrm = Approve);
    Otr otrMode();

    void setArchiveStorage(Type am, Use ap);
    Use archiveStorage(Type am);

protected:
    /**
     * \brief uniformArchivingNS creates DOM element <pref xmlns=NS></pref>.
     * This function uses static NS element of the archiving class.
     * Result can be used to embed any XEP-0136 data.
     */
    QDomElement uniformArchivingNS();
    QDomElement uniformPrefsRequest();

    typedef void (JT_Archive::*AnswerHandler)(const QDomElement&, const QDomElement&, const QString&);
    AnswerHandler chooseHandler(Preferences::QueryType type);

    void handleSet(const QDomElement&, const QDomElement&, const QString&);
    void handleGet(const QDomElement&, const QDomElement&, const QString&);
    void handleResult(const QDomElement&, const QDomElement&, const QString&);
    void handleError(const QDomElement&, const QDomElement&, const QString&);
private:
    /**
     * \brief initMetaEnums extracts enumerations information from the Qt meta system.
     * This will initialize static QMetaEnum members for further use.
     */
    void initMetaEnums();

    Scope m_Scope;

    Save m_Save;

    Otr m_Otr;

    QMap<Type, Use> m_StorageSetting;
};

#endif JT_ARCHIVE_H
