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
 * See http://xmpp.org/extensions/xep-0136.html for further details.
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

#include <KDateTime>

#include "xmpp_task.h"
#include "xmpp_xdata.h"

/**
 * @brief The JT_Archive class is an Iris task for the XMPP extension #0136, Message archiving.
 * @author Roman Nazarenko
 *
 * One can create an instance to either check or modify archiving preferences.
 * Since all XEP-0136 preferences are stored on the server, we need no resident
 * module for this extension.
 *
 * Because of this, I marked 'static' as much methods as possible -- minimal
 * on-create overhead.
 *
 * Since XEP-0136 is multipurposal, XMPP::Task::onGo() is not implemented. Instead,
 * we have several methods performing appropriate tasks.
 *
 * This class uses Qt meta system for enum-to-string conversions, so, enumerations'
 * naming is sensitive and one ought to change anything carefuly. At first, we used
 * to handle those conversions manually, but dozens of 'toSomeCrap' and 'fromSomeCrap'
 * methods always make me sick.
 */
class JT_Archive : public XMPP::Task
{
    Q_OBJECT

    /// Qt meta system macros. Those make enumerations available through Qt's
    /// QMetaEnum, allowing us to extract string names from enumerational values
    /// and vice versa.
    Q_ENUMS( TagNames )
    Q_ENUMS( AutoScope )
    Q_ENUMS( DefaultSave )
    Q_ENUMS( DefaultOtr )
    Q_ENUMS( MethodType )
    Q_ENUMS( MethodUse )
public:
    /**
     * @brief NS is an XML namespace title for XEP-0136.
     *
     * This variable is used in take() method to check if DOM element received
     * must be handled by this class. This is also used in uniformArchivingNS()
     * for proper outgoing stanzas creation.
     */
    static const QString ArchivingNS; // urn:xmpp:archive
    static const QString ResultSetManagementNS;
    //static const QString XMPP_UTC_DATETIME_FORMAT;
    //static const QString XMPP_ZONE_DATETIME_FORMAT;
    /**
     * @brief This function searches for a subtag with xmlns=#{JT_Archive::NS} parameter.
     * It's mainly a user-friendly wrapper for findSubTag() from Iris library.
     */
    static bool hasValidNS(const QDomElement&);

    /// Possible tag names for XEP-0136.
    enum TagNames {
        Auto,
        Default,
        Method,
        Item,
        Session
    };

    /// Scope of archiving setting: forever or for the current stream only.
    enum AutoScope {
        AutoScope_global,
        AutoScope_stream
    };
    /// It's set to AutoScopeGlobal by default.
    static const AutoScope defaultScope;

    /// Which part of the messaging stream should we store?
    enum DefaultSave {
        DefaultSave_false,   // Nothing
        DefaultSave_body,    // <body/>
        DefaultSave_message, // <message/>
        DefaultSave_stream   // The whole stream, byte-to-byte
    };

    /// How do we proceed Off-the-Record?
    /// This task does not support OTR now, mainly because I
    /// know nothing about it.
    /// TODO: Implement OTR support.
    enum DefaultOtr {
        DefaultOtr_concede,
        DefaultOtr_prefer,
        DefaultOtr_forbid,
        DefaultOtr_approve,
        DefaultOtr_oppose,
        DefaultOtr_require
    };

    /// Where and how hard does user prefer to store archive?
    /// This task doesn't support Manual storing.
    /// TODO: Implement manual storing
    enum MethodType {
        MethodType_auto,     // Server saves everything itself
        MethodType_local,    // Everything goes to the local storage, nothing to a server
        MethodType_manual    // Client will send messages to archive manually.
    };
    enum MethodUse {
        MethodUse_concede,   // Use if no other options available
        MethodUse_prefer,
        MethodUse_forbid
    };
    /// Special way to say 'forever'?
    static const uint defaultExpiration = (uint)-1;

    /// Enumerations meta information. We use those variables for str-to-enum
    /// conversions. Changing their names is not permitted, since they're
    /// related to enumerations' names via macroses (see jt_archive/preferences.cpp).
    static const QMetaEnum s_TagNamesEnum;
    static const QMetaEnum s_AutoScopeEnum;
    static const QMetaEnum s_DefaultSaveEnum;
    static const QMetaEnum s_DefaultOtrEnum;
    static const QMetaEnum s_MethodTypeEnum;
    static const QMetaEnum s_MethodUseEnum;

    struct CollectionsRequest {
        CollectionsRequest() {}
        CollectionsRequest(const QString &_with,
                           const KDateTime &_start = KDateTime(),
                           const KDateTime &_end = KDateTime(),
                           const QString &_after = QString())
            : with(_with),
              start(_start),
              end(_end),
              after(_after) {}

        QString with;
        KDateTime start, end;
        QString after;
    };

    struct RSMInfo {
        RSMInfo() {}
        RSMInfo(const QString &_first, const QString &_last, uint _count)
            : first(_first),
              last(_last),
              count(_count) {}

        QString first, last;
        uint count;
    };

    struct ChatInfo {
        ChatInfo() {}
        ChatInfo(const QString &_with, const KDateTime &_time)
            : with(_with),
              time(_time) {}
        QString with;
        KDateTime time;
    };

    /**
     * @param parent is a root Iris task.
     *
     * One creating this class should always use proper root task, since all the
     * facilities, such as stanzas sending and receiving, are related to this
     * root task's parametes.
     *
     * You should use Kopete's JabberAccount instance to get proper root task.
     * Something like account()->rootTask(), or client()->account()->rootTask(),
     * as done in protocols/jabber/jabbereditaccountwidget.cpp.
     */
    JT_Archive(Task *const parent);

    /**
     * @brief requestPrefs sends an empty <pref/> tag to find out stored archiving
     * preferences.
     *
     * Results will be returned via signal-slot mechanism, take a look at signals
     * section of this file for further details.
     */
    virtual void requestPrefs();

    virtual void requestCollections(const CollectionsRequest&);

    virtual void updateDefault(const DefaultSave, const DefaultOtr, const uint expiration);
    virtual void updateAuto(bool isEnabled, const AutoScope = (AutoScope)-1);
    virtual void updateStorage(const MethodType, const MethodUse);

    /**
     * @brief take is an overloaded XMPP::Task's method.
     * @return true if stanza received is supposed to be handled by XEP-0136 task and
     * is well-formed, false otherwise.
     *
     * This method checks if stanza received should be handled by this class. Look at
     * writePref() and handle*Tag() methods below for further details.
     */
    virtual bool take(const QDomElement &);

    /**
     * @brief writePrefs extracts every subtag from the QDomElement given and
     * calls writePref() with them.
     * @return true if every tag is recognized and parsed properly, false otherwise.
     */
    bool writePrefs(const QDomElement&);

    /**
     * @brief writePref parses tag name and calls proper handler from the handle{#name}Tag.
     * @return true if everything is parsed and recognized, false otherwise. If tag
     * name is correct, it returns handle*Tag function's result.
     */
    bool writePref(const QDomElement&);

    bool collectionsListReceived(const QDomElement&);

protected:
    /**
     * These functions are tags handlers. For every tag, writePref chooses proper
     * handler fron this list and calls it.
     *
     * Handlers check tag's integrity and standard compliance. If everything is right,
     * they emit an appropriate signal from the list below and return true. If optional
     * XML attribute is not set, they will set it to the default value. If optional
     * attribute is set, but invalid, signal will not be emitted and false will be
     * returned.
     */
    bool handleAutoTag(const QDomElement&);
    bool handleDefaultTag(const QDomElement&);
    bool handleItemTag(const QDomElement&);
    bool handleSessionTag(const QDomElement&);
    bool handleMethodTag(const QDomElement&);

    RSMInfo parseRSM(const QDomElement &);
    QList<ChatInfo> parseChatInfo(const QDomElement &);

    /**
     * @brief uniformArchivingNS creates DOM element <tagName xmlns=NS></tagName>.
     * This function uses static NS element of the archiving class.
     * Result can be used to embed any XEP-0136 data.
     */
    QDomElement uniformArchivingNS(const QString &tagName);
    QDomElement uniformPrefsRequest();
    QDomElement uniformAutoTag(bool, AutoScope);
    QDomElement uniformDefaultTag(DefaultSave, DefaultOtr, uint expiration);
    QDomElement uniformMethodTag(MethodType, MethodUse);
    QDomElement uniformUpdate(const QDomElement&);
    QDomElement uniformCollectionsRequest(const CollectionsRequest &params);
    QDomElement uniformRsmNS(const QString &tagName);
    QDomElement uniformRsmMax(uint max);
    QDomElement uniformSkeletonCollectionsRequest(const QString &with);
    QDomElement uniformRsm(const QString &after = QString());

signals:
    /**
     * <auto save='isEnabled' scope='scope'/>
     * Scope is an optional parameter, if not received, will be set to AutoScopeGlobal.
     */
    void automaticArchivingEnable(bool isEnabled,JT_Archive::AutoScope scope);

    /**
     * <default save='saveMode' otr='otr' expire='expire'/>
     * Expire is an optional parameter. If not received, will be set to infinity.
     */
    void defaultPreferenceChanged(JT_Archive::DefaultSave saveMode,JT_Archive::DefaultOtr otr,uint expire);

    /**
     * <method type='method' use='use'/>
     * This setting shows archiving priorities. Server usually sends three of <method/> tags,
     * so receiver should be ready to get three continuous signals.
     */
    void archivingMethodChanged(JT_Archive::MethodType method,JT_Archive::MethodUse use);

    void collectionsReceived(QList<JT_Archive::ChatInfo>, JT_Archive::RSMInfo);

protected:
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
