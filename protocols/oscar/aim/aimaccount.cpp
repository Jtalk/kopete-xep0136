//
//
// C++ Implementation: cpp
//
// Description:
//
//
// Author: Duncan Mac-Vicar Prett <duncan@kde.org>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <kdebug.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kmessagebox.h>

#include "kopetestdaction.h"

#include "aimprotocol.h"
#include "aimaccount.h"
#include "aimcontact.h"
#include "aimuserinfo.h"
#include "aimchangestatus.h"

class KopeteMetaContact;

AIMAccount::AIMAccount(KopeteProtocol *parent, QString accountID, const char *name)
	: OscarAccount(parent, accountID, name, false)
{
	kdDebug(14190) << k_funcinfo << accountID << ": Called."<< endl;
	mAwayMessage = QString::null;
	mStatus = OSCAR_OFFLINE;

	mMyself = new AIMContact(accountID, accountID, this, 0L);
	mAwayDialog = new AIMChangeStatus(engine());
	QObject::connect(mAwayDialog, SIGNAL(goAway(const int, const QString&)),
		this, SLOT(slotAwayDialogReturned(const int, const QString&)));
}

AIMAccount::~AIMAccount()
{
	delete mAwayDialog;
}

OscarContact *AIMAccount::createNewContact( const QString &contactId,
		const QString &displayName, KopeteMetaContact *parentContact )
{
	return new AIMContact(contactId, displayName, this, parentContact);
}

KActionMenu* AIMAccount::actionMenu()
{
	kdDebug(14190) << k_funcinfo << accountId() << ": Called." << endl;
	// mActionMenu is managed by KopeteAccount.  It is deleted when
	// it is no longer shown, so we can (safely) just make a new one here.
	KActionMenu *mActionMenu = new KActionMenu(accountId(),
		"aim_protocol", this, "AIMAccount::mActionMenu");

	AIMProtocol *p = AIMProtocol::protocol();

	mActionMenu->popupMenu()->insertTitle(
		mMyself->onlineStatus().iconFor(mMyself),
		i18n("%2 <%1>").arg(accountId()).arg(mMyself->displayName()));

	mActionMenu->insert(
		new KAction(p->statusOnline.caption(),
			p->statusOnline.iconFor(this), 0,
			this, SLOT(slotGoOnline()), mActionMenu,
			"AIMAccount::mActionOnline"));

	mActionMenu->insert(
		new KAction(p->statusAway.caption(),
			p->statusAway.iconFor(this), 0,
			this, SLOT(slotGoAway()), mActionMenu,
			"AIMAccount::mActionAway"));

	mActionMenu->insert(
		new KAction(p->statusOffline.caption(),
			p->statusOffline.iconFor(this),
			0, this, SLOT(slotGoOffline()), mActionMenu,
			"AIMAccount::mActionOffline"));

	mActionMenu->popupMenu()->insertSeparator();

	mActionMenu->insert(
		KopeteStdAction::contactInfo(this, SLOT(slotEditInfo()),
			mActionMenu, "AIMAccount::mActionEditInfo"));

	return mActionMenu;
}

void AIMAccount::initSignals()
{
	// Got my user info
	QObject::connect(
		engine(), SIGNAL(gotMyUserInfo(UserInfo &)),
		this, SLOT(slotGotMyUserInfo(UserInfo &)));

	// Got warning
	QObject::connect(
		engine(), SIGNAL(gotWarning(int,QString)),
		this, SLOT(slotGotWarning(int,QString)));
}

void AIMAccount::slotGotMyUserInfo(UserInfo &newInfo)
{
	mUserInfo = newInfo;
}

// FIXME: Called from AIMUserInfo
void AIMAccount::setUserProfile(QString profile)
{
	// Tell the engine to set the profile
	engine()->setMyProfile( profile );
	// Save the user profile
	setPluginData(protocol(), "Profile", profile);
}

// Called when we have been warned
void AIMAccount::slotGotWarning(int newlevel, QString warner)
{
	kdDebug(14190) << k_funcinfo << "Called." << endl;

	//this is not a natural decrease in level
	if (mUserInfo.evil < newlevel)
	{
		QString warnMessage;
		if(warner.isNull())
			warnMessage = i18n("anonymously");
		else
			warnMessage = i18n("...warned by...", "by %1").arg(warner);

		// Construct the message to be shown to the user
		QString message =
			i18n("You have been warned %1. Your new warning level is %2%.").arg(
				warnMessage).arg(newlevel);

		KMessageBox::sorry(0L,message);
	}
	mUserInfo.evil = newlevel;
}

void AIMAccount::slotEditInfo()
{
	AIMUserInfo *myInfo;

	myInfo = new AIMUserInfo(
		accountId(), accountId(),
		this, engine()->getMyProfile());

	myInfo->exec(); // This is a modal dialog
}

void AIMAccount::setAway(bool away, const QString &awayReason)
{
	kdDebug(14180) << k_funcinfo << accountId() << ": setAway()" << endl;

	if(away)
		setStatus(OSCAR_AWAY, awayReason);
	else
		setStatus(OSCAR_ONLINE, QString::null);
}


void AIMAccount::setStatus(const unsigned long status,
	const QString &awayMessage)
{
	kdDebug(14180) << k_funcinfo << "new status=" << status << ", old status=" << mStatus << endl;
	mStatus = status;

	if(!awayMessage.isNull())
		mAwayMessage = awayMessage;

	if (isConnected())
	{
		engine()->sendAIMAway((status==OSCAR_AWAY), awayMessage);
	}
}

void AIMAccount::slotAwayDialogReturned(const int status, const QString &message)
{
	setStatus(status,message);
}

#include "aimaccount.moc"
// vim: set noet ts=4 sts=4 sw=4:
