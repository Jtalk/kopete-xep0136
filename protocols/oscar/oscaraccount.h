/*
  OscarAccount - Oscar Protocol Account

  Copyright (c) 2002 by Chris TenHarmsel <tenharmsel@staticmethod.net>

  Kopete    (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

  *************************************************************************
  *                                                                       *
  * This program is free software; you can redistribute it and/or modify  *
  * it under the terms of the GNU General Public License as published by  *
  * the Free Software Foundation; either version 2 of the License, or     *
  * (at your option) any later version.                                   *
  *                                                                       *
  *************************************************************************

*/

#ifndef OSCARACCOUNT_H
#define OSCARACCOUNT_H

#include <qwidget.h>
#include <qdict.h>
#include <qstring.h>

#include "kopeteaccount.h"
#include "oscarsocket.h"
#include "oscarprotocol.h"
#include "xautolock.h"

class KAction;

class KopeteContact;
class KopeteGroup;

class OscarChangeStatus;
class OscarContact;
class OscarDebugDialog;

class AIMBuddyList;

class OscarAccount : public KopeteAccount
{
	Q_OBJECT

	public:
		OscarAccount(KopeteProtocol *parent, QString accountID, const char *name=0L);
		virtual ~OscarAccount();

		/** Connects this account */
		void connect();

		/** Disconnects this account  */
		void disconnect();

		/** Sets the account away */
		virtual void setAway( bool away );

		/** Accessor method for this account's contact */
		virtual KopeteContact* myself() const;

		/** Sets the user's profile */
		virtual void setUserProfile( QString profile );

		/** Accessor method for our engine object */
		virtual OscarSocket* getEngine();

		/** Accessor method for the action menu */
		KActionMenu* actionMenu();

		/** Gets the next random new buddy num */
		int randomNewBuddyNum();

		/** Gets the next random new group num */
		int randomNewGroupNum();

		/** Gets the internal buddy list */
		AIMBuddyList *internalBuddyList();

		/** Sets the port we connect to */
		void setPort( int port );

		/** Sets the server we connect to */
		void setServer( QString server );

		/** returns wether this account is used to connect to ICQ or AIM
		* true = ICQ
		* false = AIM
		*/
		bool isICQ();

	public slots:
		/** Slot for telling this account to go online */
		void slotGoOnline();
		/** Slot for telling this account to go offline */
		void slotGoOffline();
		/** Slot for telling this account to go away */
		void slotGoAway();
		/** Slot for editing our info */
		void slotEditInfo();
		/** Slot for showing the debug dialog */
		void slotShowDebugDialog();

	protected slots:
		/** Called when we get disconnected */
		void slotDisconnected();

		/** Called when a group is added for this account */
		void slotGroupAdded(KopeteGroup* group);

		/** Called when a group is removed for this account */
		void slotGroupRemoved( const QString& groupName, uint groupNumber );

		/**
		* Called when a group is renamed for this account
		* @param groupName The new group name
		* @param groupNumber The group number, shouldn't change
		*/
		void slotGroupRenamed( const QString& groupName, uint groupNumber );

		/** Called when the contact list renames a group */
		void slotKopeteGroupRenamed( KopeteGroup *group );

		/** Called when the contact list removes a group */
		void slotKopeteGroupRemoved( KopeteGroup *group );

		/**
		* Called when our status on the server has changed
		* This is called when disconnected too
		*/
		void slotOurStatusChanged( const KopeteOnlineStatus &newStatus );

		/**
		* Called when we get a contact list from the server
		* The parameter is a qmap with the contact names as keys
		* the value is another map with keys "name", "group"
		*/
		void slotGotServerBuddyList( AIMBuddyList& buddyList );

		/** Called when we've received an IM */
		void slotGotIM( QString message, QString sender, bool isAuto );

		/** Called when we have been warned */
		void slotGotWarning(int newlevel, QString warner);

		/** Called when we get a request for a direct IM session with @sn */
		void slotGotDirectIMRequest(QString sn);

		/** Called when the engine notifies us that it got our user info */
		void slotGotMyUserInfo(UserInfo newInfo);

		/** Called when there is no activity for a certain amount of time  */
		void slotIdleTimeout();

		/** Called when there is mouse/keyboard activity */
		void slotIdleActivity();

		/** Displays an error dialog with the given text */
		void slotError(QString errmsg, int errorCode);

	protected:
		/** Adds a contact to a meta contact */
		virtual bool addContactToMetaContact(const QString &contactId,
			const QString &displayName, KopeteMetaContact *parentContact);

		/**
		* Adds a contact to the internal list.
		* This means that the contact is already
		* on the server-side list
		*/
		virtual void addServerContact(AIMBuddy *buddy);

		/** Initializes our actions */
		virtual void initActions();

		/** Initializes our action menu */
		virtual void initActionMenu();

		/** Initializes the engine */
		virtual void initEngine();

		/** Initializes the signals */
		virtual void initSignals();

	protected:
		/** Flag for remembering the password */
		bool m_rememberPassword;

		/** Our Internal buddy list (from the server) */
		AIMBuddyList *m_internalBuddyList;

		/** Our contact */
		OscarContact *m_myself;

		/** Our OSCAR socket object */
		OscarSocket *m_engine;

		/** Our UserInfo */
		UserInfo m_userInfo;

		/** Our action menu */
		KActionMenu *m_actionMenu;

		/** Go online action */
		KAction *m_actionGoOnline;

		/** Go offline action */
		KAction *m_actionGoOffline;

		/** Go away action */
		KAction *m_actionGoAway;

		/** Edit user info */
		KAction *m_actionEditInfo;

		/** Show debug dialog action */
		KAction *m_actionShowDebug;

		/** Our away dialog */
		OscarChangeStatus *m_awayDialog;

		/** Our debug dialog */
		OscarDebugDialog *m_debugDialog;

		/** Random new buddy number for the engine */
		int m_randomNewBuddyNum;

		/** Random new group number for the engine */
		int m_randomNewGroupNum;

		/** Idleness manager */
		XAutoLock m_idleMgr;
};
#endif
// vim: set noet ts=4 sts=4 sw=4:
