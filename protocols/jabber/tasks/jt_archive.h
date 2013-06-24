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
#include <QtCore/QVariant>
#include <QtCore/QMetaEnum>
#include <QtCore/QMetaObject>
#include <QtXml/QDomElement>

#include "xmpp_task.h"
#include "xmpp_xdata.h"


class JT_Archive : public XMPP::Task
{
    Q_OBJECT
    Q_ENUMS( TagNames )
    Q_ENUMS( AutoScope )
    Q_ENUMS( DefaultSave )
    Q_ENUMS( DefaultOtr )
    Q_ENUMS( MethodType )
    Q_ENUMS( MethodUse )
public:
    static const QString NS; // urn:xmpp:archive
    static bool hasValidNS(QDomElement);

    /// Possible tag names
    enum TagNames {
        Auto,
        Default,
        Method,
        Item,
        Session
    };

    /// Scope of archiving setting: forever or for the current stream only
    enum AutoScope {
        AutoScopeGlobal,
        AutoScopeStream
    };
    static const AutoScope defaultScope;

    /// Which part of the messaging stream should we store?
    enum DefaultSave {
        DefaultSaveFalse,
        DefaultSaveBody,
        DefaultSaveMessage,
        DefaultSaveStream
    };

    /// How do we proceed Off-the-Record?
    enum DefaultOtr {
        DefaultOtrConcede,
        DefaultOtrPrefer,
        DefaultOtrForbid,
        DefaultOtrApprove,
        DefaultOtrOppose,
        DefaultOtrRequire
    };

    /// Where and how hard does user prefer to store archive?
    enum MethodType {
        MethodTypeAuto,
        MethodTypeLocal,
        MethodTypeManual
    };
    enum MethodUse {
        MethodUseConcede,
        MethodUsePrefer,
        MethodUseForbid
    };

    static const uint defaultExpiration = (uint)-1;


    bool writePrefs(const QDomElement&);
    bool writePref(const QDomElement&);
    QString readPref(const QString &name);
    QDomElement uniformDomElement(const QString &name);

protected:
    bool setPref(const QString& name, const QVariant& value);

    bool handleAutoTag(const QDomElement&);
    bool handleDefaultTag(const QDomElement&);
    bool handleItemTag(const QDomElement&);
    bool handleSessionTag(const QDomElement&);
    bool handleMethodTag(const QDomElement&);

signals:
    void automaticArchivingEnable(bool,JT_Archive::AutoScope scope); // <auto save='true|false' scope='global|stream'/>
    void defaultPreferenceChanged(JT_Archive::DefaultSave saveMode,JT_Archive::DefaultOtr otr,uint expire); // <default save='#Save' otr='#Otr' expire='12345'/>
    void archivingMethodChanged(JT_Archive::MethodType method,JT_Archive::MethodUse use); // <method type='auto|manual|local' use='concede|prefer|forbid'/>

public:
    static const QMetaEnum s_TagNamesEnum;
    static const QMetaEnum s_AutoScopeEnum;
    static const QMetaEnum s_DefaultSaveEnum;
    static const QMetaEnum s_DefaultOtrEnum;
    static const QMetaEnum s_MethodTypeEnum;
    static const QMetaEnum s_MethodUseEnum;

    JT_Archive(Task *const parent);

    virtual void requestPrefs();
    virtual bool take(const QDomElement &);

protected:
    /**
     * \brief uniformArchivingNS creates DOM element <pref xmlns=NS></pref>.
     * This function uses static NS element of the archiving class.
     * Result can be used to embed any XEP-0136 data.
     */
    QDomElement uniformArchivingNS(const QString &tagName);
    QDomElement uniformPrefsRequest();

    typedef bool (JT_Archive::*AnswerHandler)(const QDomElement&, const QDomElement&, const QString&);
    AnswerHandler chooseHandler(const QDomElement&);

private:
    bool handleSet(const QDomElement&, const QDomElement&, const QString&);
    bool handleGet(const QDomElement&, const QDomElement&, const QString&);
    bool handleResult(const QDomElement&, const QDomElement&, const QString&);
    bool handleError(const QDomElement&, const QDomElement&, const QString&);
    bool skip(const QDomElement&, const QDomElement&, const QString&) { return false; }
    bool acknowledge(const QDomElement&, const QDomElement&, const QString&) { return true; }
};

#endif
