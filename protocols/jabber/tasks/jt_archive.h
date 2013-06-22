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

#include "xmpp_task.h"
#include "xmpp_xdata.h"

#include "jt_archive/preferences.h"

class QDomElement;
using XMPP::Task;

class JT_Archive : public Task
{
    Q_OBJECT
public:
    static const QString NS; // urn:xmpp:archive
    static bool hasValidNS(QDomElement);

    enum QueryType {
        Get,            // <iq type='get'>
        Set,            // <iq type=... etc.
        Result,
        Acknowledgement, // Empty iq. Server uses them to show us an acknowlegement of changes previously sent to it.
        Error,
        NO_ARCHIVE      // This stanza is not supposed to be handled by an archiving task
    };

    JT_Archive(Task *const parent);

    virtual void onGo();
    virtual bool take(const QDomElement &);

    //strToPref(const QString&);

    // Setters and getters.
    // I skip any enum validation, because if one wish to shoot his leg, no one
    // can ever stop him from doing this.
//#error SETTING_VALUES?

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

    bool handleSet(const QDomElement&, const QDomElement&, const QString&);
    bool handleGet(const QDomElement&, const QDomElement&, const QString&);
    bool handleResult(const QDomElement&, const QDomElement&, const QString&);
    bool handleError(const QDomElement&, const QDomElement&, const QString&);
    bool skip(const QDomElement&, const QDomElement&, const QString&) { return false; }
    bool acknowledge(const QDomElement&, const QDomElement&, const QString&) { return true; }
private:
    JT_Archive_Helper::Preferences *m_preferences;
};

#endif
