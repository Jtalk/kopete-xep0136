/*
    chatview.cpp - Chat View

    Copyright (c) 2002      by Olivier Goffart       <ogoffart@tiscalinet.be>
    Copyright (c) 2002-2003 by Martijn Klingens      <klingens@kde.org>

    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "chatview.h"

#include <qclipboard.h>
#include <qheader.h>

#include <dom/dom_doc.h>
#include <dom/dom_element.h>
#include <dom/html_base.h>
#include <dom/html_document.h>
#include <dom/html_inline.h>
#include <kapplication.h>
#include <kcompletion.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kfiledialog.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kiconeffect.h>
#include <kiconloader.h>
#include <klibloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <krootpixmap.h>
#include <krun.h>
#include <ktempfile.h>
#include <kwin.h>

#include "kopetechatwindow.h"
#include "krichtexteditpart.cpp"
#include "kopetemessagemanager.h"
#include "kopetemetacontact.h"
#include "kopetepluginmanager.h"
#include "kopeteprefs.h"
#include "kopeteprotocol.h"
#include "kopetexsl.h"
#include "kopeteaccount.h"

#include <ktabwidget.h>

class KopeteChatViewPrivate
{
public:
	KopeteXSLT *xsltParser;
};

ChatView::ChatView( KopeteMessageManager *mgr, const char *name )
	 : KDockMainWindow( 0L, name, 0L ), KopeteView( mgr ), editpart(0)
{
	d = new KopeteChatViewPrivate;

	d->xsltParser = new KopeteXSLT( KopetePrefs::prefs()->styleContents() );

	hide();

	//Create the view dock widget (KHTML Part), and set it to no docking (lock it in place)
	viewDock = createDockWidget(QString::fromLatin1( "viewDock" ), QPixmap(),
		0L,QString::fromLatin1("viewDock"), QString::fromLatin1(" "));
	chatView = new KHTMLPart( viewDock, "chatView" );

	//Security settings, we don't need this stuff
	chatView->setJScriptEnabled( false ) ;
	chatView->setJavaEnabled( false );
	chatView->setPluginsEnabled( false );
	chatView->setMetaRefreshEnabled( false );

	chatView->begin();
	chatView->write( QString::fromLatin1( "<html><head><style>") + styleHTML() +
		QString::fromLatin1("</style></head><body></body></html>") );
	chatView->end();

	htmlWidget = chatView->view();
	htmlWidget->setFocusPolicy( NoFocus );

	viewDock->setWidget(htmlWidget);
	viewDock->setDockSite(KDockWidget::DockBottom);
	viewDock->setEnableDocking(KDockWidget::DockNone);

	//Create the bottom dock widget, with the edit area, statusbar and send button
	editDock = createDockWidget( QString::fromLatin1( "editDock" ), QPixmap(),
		0L, QString::fromLatin1("editDock"), QString::fromLatin1(" ") );

	editpart = new KopeteRichTextEditPart( editDock, "kopeterichtexteditpart", mgr->protocol()->supportsRichText() );

	m_edit = static_cast<KTextEdit*>( editpart->widget() );

	//Set params on the edit widget
	m_edit->setMinimumSize( QSize( 75, 20 ) );
	m_edit->setWordWrap( QTextEdit::WidgetWidth );
	m_edit->setWrapPolicy( QTextEdit::AtWhiteSpace );
	m_edit->setAutoFormatting( QTextEdit::AutoNone );

	//Make the edit area dockable for now
	editDock->setWidget( m_edit );
	editDock->setDockSite( KDockWidget::DockNone );
	editDock->setEnableDocking(KDockWidget::DockBottom);

	//Set the view as the main widget
	setMainDockWidget( viewDock );
	setView(viewDock);

	// some signals and slots connections
	connect( m_edit, SIGNAL( textChanged()), this, SLOT( slotTextChanged() ) );
	connect( KopetePrefs::prefs(), SIGNAL(transparencyChanged()),
		this, SLOT( slotTransparencyChanged() ) );
	connect( KopetePrefs::prefs(), SIGNAL(messageAppearanceChanged()),
		this, SLOT( slotAppearanceChanged() ) );
	connect( KopetePrefs::prefs(), SIGNAL(windowAppearanceChanged()),
		this, SLOT( slotRefreshView() ) );

	// Timers for typing notifications
	m_typingRepeatTimer = new QTimer(this, "m_typingRepeatTimer");
	m_typingStopTimer   = new QTimer(this, "m_typingStopTimer");

	m_remoteTypingMap.setAutoDelete( true );

	connect( m_typingRepeatTimer, SIGNAL( timeout() ),
		this, SLOT( slotRepeatTimer() ) );
	connect( m_typingStopTimer,   SIGNAL( timeout() ),
		this, SLOT( slotStopTimer() ) );

	connect( mgr, SIGNAL( displayNameChanged() ),
		this, SLOT( slotChatDisplayNameChanged() ) );
	connect( mgr, SIGNAL( contactAdded(const KopeteContact*, bool) ),
		this, SLOT( slotContactAdded(const KopeteContact*, bool) ) );
	connect( mgr, SIGNAL( contactRemoved(const KopeteContact*, const QString&) ),
		this, SLOT( slotContactRemoved(const KopeteContact*, const QString&) ) );

	connect ( chatView->browserExtension(), SIGNAL( openURLRequestDelayed( const KURL &, const KParts::URLArgs & ) ),
		this, SLOT( slotOpenURLRequest( const KURL &, const KParts::URLArgs & ) ) );

	connect( chatView, SIGNAL(popupMenu(const QString &, const QPoint &)),
		this, SLOT(slotRightClick(const QString &, const QPoint &)) );
	connect( htmlWidget, SIGNAL(contentsMoving(int,int)),
		this, SLOT(slotScrollingTo(int,int)) );

	setFocusProxy( m_edit );
	m_edit->setFocus();

	// Finalize
	historyPos = -1;
	m_type = KopeteMessage::Chat;
	m_mainWindow = 0L;
	membersDock = 0L;
	backgroundFile = QString::null;
	root = 0L;
	isActive = false;
	m_tabBar = 0L;
	messageId = 0;
	membersStatus = Smart;
	bgChanged = false;
	m_tabState=Normal;
	//valgrind complained about this - initialize it to true
	visibleMembers = true;

//	m_icon = SmallIcon( mgr->protocol()->pluginIcon() );
//	m_iconLight = KIconEffect().apply( m_icon, KIconEffect::ToGamma, 0.5, Qt::white, true );
	m_sendInProgress = false;
	scrollPressed = false;
	mComplete = new KCompletion();
	mComplete->setIgnoreCase( true );
	mComplete->setOrder( KCompletion::Weighted );

	//initActions
	copyAction  = KStdAction::copy( this, SLOT(copy()), actionCollection());
	saveAction = KStdAction::save ( this, SLOT(save()), actionCollection());
	printAction = KStdAction::print ( this, SLOT(print()),actionCollection());
	closeAction = KStdAction::close ( this, SLOT(closeView()),actionCollection());
	copyURLAction = new KAction( i18n("Copy Link Location"),QString::fromLatin1("editcopy"),0,this, SLOT(slotCopyURL()),actionCollection());

	setCaption( m_manager->displayName(), false );

	//Restore docking positions
	readOptions();

	//Show chat members
	createMembersList();

	slotTransparencyChanged();
}

ChatView::~ChatView()
{
	emit( closing( static_cast<KopeteView*>(this) ) );

	saveOptions();

	delete mComplete;

	delete d;
}

void ChatView::raise(bool activate)
{
	//this shouldn't change the focus. When the window is reased when a new mesage arrive
	// if i am coding, or talking to someone else, i want to end my sentence before switch to
	// the other chat. i just want to KNOW and SEE the other chat to switch to it right later
	// (exepted if activate==true)

	if(!m_mainWindow || !m_mainWindow->isActiveWindow() || activate)
		makeVisible();

	if( !KWin::info( m_mainWindow->winId() ).onAllDesktops )
		KWin::setOnDesktop( m_mainWindow->winId(), KWin::currentDesktop() );

	m_mainWindow->show();
	//raise() and show() should normaly deIconify the window. but it doesn't do here due
	// to a bug in QT or in KDE  (qt3.1.x or KDE 3.1.x) then, i have to call KWin's method
	if(m_mainWindow->isMinimized())
		KWin::deIconifyWindow(m_mainWindow->winId() );
	m_mainWindow->raise();

	/* Removed Nov 2003
	According to Zack, the user double-clicking a contact is not valid reason for a non-pager
	to grab window focus. While I don't agree with this, and it runs contradictory to every other
	IM out there, commenting this code out to agree with KWin policy.

	Redirect any bugs relating to the widnow now not grabbing focus on clicking a contact to KWin.
		- Jason K
	*/

	//Will not activate window if user was typing
	if(activate)
#if KDE_VERSION < KDE_MAKE_VERSION( 3, 1, 90 )
		KWin::setActiveWindow( m_mainWindow->winId() );
#else
		KWin::activateWindow( m_mainWindow->winId() );
#endif
}

void ChatView::slotScrollingTo( int /*x*/, int y)
{
	int scrolledTo = y + htmlWidget->visibleHeight();
	if( scrolledTo >= ( htmlWidget->contentsHeight() - 10 ) )
		scrollPressed = false;
	else
		scrollPressed = true;
}

void ChatView::save()
{
	QString fileName = KFileDialog::getSaveFileName ( QString::null, QString::fromLatin1( "text/xml" ),
		this, i18n( "Save Conversation" ) );

	if ( fileName.isEmpty() )
		return;

	QFile file ( fileName );
	if ( file.open(IO_WriteOnly) )
	{
		QTextStream stream ( &file );
		QString xmlString;
		for( MessageMap::Iterator it = messageMap.begin(); it != messageMap.end(); ++it)
			xmlString += (*it).asXML().toString();

		stream << QString::fromLatin1("<document>") << xmlString << QString::fromLatin1("</document>");
		file.close(); // maybe unneeded but I like to close opened files ;)
	}
	else
	{
		KMessageBox::queuedMessageBox( this, KMessageBox::Error,
				i18n("<qt>Could not open <b>%1</b> for writing.</qt>").arg(fileName), // Message
				i18n("Error While Saving") ); //Caption
	}
}

void ChatView::makeVisible()
{
	if( !m_mainWindow )
	{
		m_mainWindow = KopeteChatWindow::window( m_manager );
		if( root )
			root->repaint( true );
	}

	if( !m_mainWindow->isVisible() )
		m_mainWindow->show();

	m_mainWindow->setActiveView( this );
}

bool ChatView::isVisible()
{
	return ( m_mainWindow && m_mainWindow->isVisible() && ( isActive || docked() ) );
}

bool ChatView::closeView( bool force )
{
	int response = KMessageBox::Continue;

	if( !force )
	{
		if( m_manager->members().count() > 1 )
		{
			QString shortCaption = m_captionText;
			if( shortCaption.length() > 40 )
				shortCaption = shortCaption.left( 40 ) + QString::fromLatin1("...");

			response = KMessageBox::warningContinueCancel(this, i18n("<qt>You are about to leave the group chat session <b>%1</b>.<br>"
				"You will not receive future messages from this conversation.</qt>").arg(shortCaption), i18n("Closing Group Chat"),
				i18n("Cl&ose Chat"), i18n("Do not ask me this again"));
		}

		if( !unreadMessageFrom.isNull() && ( response == KMessageBox::Continue ) )
		{
			response = KMessageBox::warningContinueCancel(this, i18n("<qt>You have received a message from <b>%1</b> in the last "
				"second. Are you sure you want to close this chat?</qt>").arg(unreadMessageFrom), i18n("Unread Message"),
				i18n("Cl&ose Chat"), i18n("Do not ask me this again"));
		}

		if( m_sendInProgress  && ( response == KMessageBox::Continue ) )
		{
			response = KMessageBox::warningContinueCancel(this, i18n("You have a message send in progress, which will be "
				"aborted if this chat is closed. Are you sure you want to close this chat?"), i18n("Message in Transit"),
				i18n("Cl&ose Chat"), i18n("Do not ask me this again"));
		}
	}

	if( response == KMessageBox::Continue )
	{
		// Remove the widget from the window it's attached to
		// and schedule it for deletion
		if( m_mainWindow )
			m_mainWindow->detachChatView( this );
		deleteLater();

		return true;
	}

	return false;
}

void ChatView::setTabState( KopeteTabState newState )
{
	if( newState == Undefined )
		newState = m_tabState;
	else if( newState != Typing &&  (  newState!=Changed || (m_tabState != Message && m_tabState != Highlighted) ) && ( newState != Message ||  m_tabState != Highlighted ) )
		m_tabState = newState;

	newState = m_remoteTypingMap.isEmpty() ? m_tabState : Typing ;

	if( m_tabBar )
	{
		switch( newState )
		{
			case Highlighted:
				m_tabBar->setTabColor( this, Qt::blue );
				break;

			case Message:
				m_tabBar->setTabColor( this, Qt::red );
				break;

			case Changed:
				m_tabBar->setTabColor( this, Qt::darkRed );
				break;

			case Typing:
				m_tabBar->setTabColor( this, Qt::darkGreen );
				break;

			case Normal:
			default:
				m_tabBar->setTabColor( this, KGlobalSettings::textColor() );
				break;
		}
	}

	if( newState!= Typing )
		setStatus( i18n( "One person in the chat", "%n people in the chat", memberContactMap.count() ) );
}

void ChatView::setMainWindow( KopeteChatWindow* parent )
{
	m_mainWindow = parent;
	if( root )
	{
		disconnect(root, SIGNAL(backgroundUpdated(const QPixmap &)), this, SLOT(slotUpdateBackground(const QPixmap &)));
		delete root;
		root = 0L;
		slotTransparencyChanged();
	}
}

void ChatView::createMembersList(void)
{
	if( !membersDock )
	{
		//Create the chat members list
		membersDock = createDockWidget( QString::fromLatin1( "membersDock" ), QPixmap(), 0L,
			QString::fromLatin1( "membersDock" ), QString::fromLatin1( " " ) );
		membersList = new KListView(this);
		membersList->setAllColumnsShowFocus( true );
		membersList->addColumn( QString::null, 18);
		membersList->addColumn( i18n("Chat Members"), -1 );
		membersList->setSorting( 0, true );
		membersList->header()->setStretchEnabled( true, 1 );
		membersList->header()->hide();
		//Add the contacts that are in the message manager
		KopeteContact *contact;
		KopeteContactPtrList chatMembers = m_manager->members();

		for ( contact = chatMembers.first(); contact; contact = chatMembers.next() )
			slotContactAdded( contact, true );

		slotContactAdded( m_manager->user(), true);

		membersDock->setWidget(membersList);

		KopeteContactPtrList members = m_manager->members();
		membersStatus = members.first()->metaContact() ?  static_cast<MembersListPolicy>(
			members.first()->metaContact()->pluginData( m_manager->protocol(),
				QString::fromLatin1("MembersListPolicy") ).toInt() )        : Smart;

		if( membersStatus == Smart )
			visibleMembers = ( memberContactMap.count() > 2 );
		else
			visibleMembers = ( membersStatus == Visible );

		placeMembersList( membersDockPosition );

		//Connect the popup menu
		connect( membersList, SIGNAL( contextMenu( KListView*, QListViewItem *, const QPoint &) ),
			SLOT( slotContactsContextMenu(KListView*, QListViewItem *, const QPoint & ) ) );
	}
}

void ChatView::toggleMembersVisibility()
{
	if( membersDock )
	{
		visibleMembers = !visibleMembers;
		membersStatus = visibleMembers ? Visible : Hidden;
		placeMembersList( membersDockPosition );
		KopeteContactPtrList members = m_manager->members();
		if(members.first()->metaContact())
			members.first()->metaContact()->setPluginData( m_manager->protocol(),
				QString::fromLatin1("MembersListPolicy"), QString::number(membersStatus) );
		refreshView();
	}
}

void ChatView::placeMembersList( KDockWidget::DockPosition dp )
{
	kdDebug(14000) << k_funcinfo << "Members list policy " << membersStatus <<
			", visible " << visibleMembers << endl;

	if ( visibleMembers )
	{
		membersDockPosition = dp;

		// look up the dock width
		int dockWidth;
		KGlobal::config()->setGroup( QString::fromLatin1("ChatViewDock") );
		if( membersDockPosition == KDockWidget::DockLeft )
			dockWidth = KGlobal::config()->readNumEntry(
				QString::fromLatin1("membersDock,viewDock:sepPos"), 30);
		else
			dockWidth = KGlobal::config()->readNumEntry(
				QString::fromLatin1("viewDock,membersDock:sepPos"), 70);

		// Make sure it is shown then place it wherever
		membersDock->setEnableDocking( KDockWidget::DockLeft | KDockWidget::DockRight );
		membersDock->manualDock( viewDock, membersDockPosition, dockWidth );
		membersDock->show();
		membersDock->setEnableDocking( KDockWidget::DockNone );
	}
	else
	{
		// Dock it to the desktop then hide it
		membersDock->undock();
		membersDock->hide();

		if ( root )
			root->repaint( true );
	}

	if( isActive )
		m_mainWindow->updateMembersActions();

	refreshView();
}

void ChatView::slotContactsContextMenu( KListView*, QListViewItem *item, const QPoint &point )
{
	KopeteContactLVI *contactLVI = dynamic_cast<KopeteContactLVI*>( item );
	if (contactLVI) {
		KPopupMenu *p = ((KopeteContact*)contactLVI->contact())->popupMenu( m_manager );
		connect(p,SIGNAL(aboutToHide()),p,SLOT(deleteLater()));
		p->popup( point );
	}
}

void ChatView::remoteTyping( const KopeteContact *c, bool isTyping )
{
	// Make sure we (re-)add the timer at the end, because the slot will
	// remove the first timer
	// And yes, the const_cast is a bit ugly, but it's only used as key
	// value in this dictionary (no indirections) so it's basically
	// harmless. Unfortunately there's no QConstPtrDictionary in Qt...
	void *key = const_cast<KopeteContact *>( c );
	m_remoteTypingMap.remove( key );
	if( isTyping )
	{
		m_remoteTypingMap.insert( key, new QTimer(this) );
		connect( m_remoteTypingMap[ key ], SIGNAL( timeout() ), SLOT( slotRemoteTypingTimeout() ) );
		m_remoteTypingMap[ key ]->start( 6000, true );
	}

	// Loop through the map, constructing a string of people typing
	QStringList typingList;
	QString statusTyping;
	QPtrDictIterator<QTimer> it( m_remoteTypingMap );

	for( ; it.current(); ++it )
	{
		KopeteContact *c=static_cast<KopeteContact*>(it.currentKey());
		typingList.append( c->metaContact() ? c->metaContact()->displayName() : c->displayName() );
	}

	statusTyping = typingList.join( QString::fromLatin1( ", " ) );

	// Update the status area
	if( !typingList.isEmpty() )
	{
		setStatus ( i18n( "%1 is typing a message", "%1 are typing a message", typingList.count() ).arg( statusTyping ) );
		setTabState( Typing );
	}
	else
	{
		setTabState();
	}
}

void ChatView::setStatus( const QString &status )
{
	m_status = status;
	if( isActive )
		m_mainWindow->setStatus( status );
}

void ChatView::pageUp()
{
	htmlWidget->scrollBy( 0, -htmlWidget->visibleHeight() );
}

void ChatView::pageDown()
{
	htmlWidget->scrollBy( 0, htmlWidget->visibleHeight() );
}

void ChatView::nickComplete()
{
	int firstSpace, lastSpace, para = 1, parIdx = 1;
	m_edit->getCursorPosition( &para, &parIdx);
	QString txt = editpart->text( Qt::PlainText );

	if( parIdx > 0 )
	{
		firstSpace = txt.findRev( QRegExp( QString::fromLatin1("\\s\\S+") ), parIdx - 1 ) + 1;
		lastSpace = txt.find( QRegExp( QString::fromLatin1("[\\s\\:]") ), firstSpace );
		if( lastSpace == -1 )
			lastSpace = txt.length();

		QString word = txt.mid( firstSpace, lastSpace - firstSpace );
		QString m_Match;

		kdDebug() << k_funcinfo << word << endl;

		if( word != m_lastMatch )
		{
			m_Match = mComplete->makeCompletion( word );
			m_lastMatch = QString::null;
			parIdx -= word.length();
		}
		else
		{
			m_Match = mComplete->nextMatch();
			parIdx -= m_lastMatch.length();
		}

		if( !m_Match.isNull() && !m_Match.isEmpty() )
		{
			QString rightText = txt.right( txt.length() - lastSpace );

			if( para == 0 && firstSpace == 0 )
			{
				rightText = m_Match + QString::fromLatin1(": ");
				parIdx += 2;
			}
			else
				rightText = m_Match + rightText;

			m_edit->removeParagraph( para );
			m_edit->insertParagraph( txt.left(firstSpace) + rightText, para);
			m_edit->setCursorPosition( para, parIdx + m_Match.length() );
			m_lastMatch = m_Match;
		}
	}
}

void ChatView::slotChatDisplayNameChanged()
{
	//This fires whenever a contact or MC changes displayName, so only
	//update the caption if it changed to avioud unneeded updates that
	//could cause flickering
	QString chatName = m_manager->displayName();
	if( chatName != m_captionText )
		setCaption( chatName, true );
}

void ChatView::slotContactNameChanged( const QString &oldName, const QString &newName )
{
	if( KopetePrefs::prefs()->showEvents() )
		sendInternalMessage( i18n( "%1 has changed their nickname to %2" ).
#if QT_VERSION < 0x030200
			arg( oldName ).arg( newName )
#else
			arg( oldName, newName )
#endif
		);
	mComplete->removeItem( oldName );
	mComplete->addItem( newName );
}

void ChatView::slotContactAdded(const KopeteContact *c, bool surpress)
{
	if( !memberContactMap.contains(c) )
	{
		QString contactName;
		contactName = c->displayName();
		connect( c, SIGNAL( displayNameChanged( const QString &,const QString & ) ),
			this, SLOT( slotContactNameChanged( const QString &,const QString & ) ) );

		mComplete->addItem( contactName );

		connect( c, SIGNAL( onlineStatusChanged( KopeteContact *, const KopeteOnlineStatus & , const KopeteOnlineStatus &) ),
			this, SLOT( slotContactStatusChanged( KopeteContact *, const KopeteOnlineStatus &, const KopeteOnlineStatus & ) ) );

		if( !surpress && memberContactMap.count() > 1 )
			sendInternalMessage(  i18n("%1 has joined the chat.").arg(contactName) );

		memberContactMap.insert(c, new KopeteContactLVI( this, c, membersList ) );

		if( membersStatus == Smart && membersDock )
		{
			bool currStatus = ( memberContactMap.count() > 2 );
			if( currStatus != visibleMembers )
			{
				visibleMembers = currStatus;
				placeMembersList( membersDockPosition );
			}
		}
	}

	setTabState();
	emit updateStatusIcon( this );
}

void ChatView::slotContactRemoved( const KopeteContact *contact, const QString &reason )
{
	if ( memberContactMap.contains( contact ) && contact != m_manager->user() )
	{
		m_remoteTypingMap.remove( const_cast<KopeteContact *>( contact ) );

		disconnect( contact, SIGNAL( displayNameChanged( const QString &, const QString & ) ),
			this, SLOT( slotContactNameChanged( const QString &, const QString & ) ) );

		QString contactName = contact->displayName();
		mComplete->removeItem( contactName );

		disconnect( contact, SIGNAL( onlineStatusChanged( KopeteContact *, const KopeteOnlineStatus &, const KopeteOnlineStatus & ) ),
			this, SLOT( slotContactStatusChanged( KopeteContact *, const KopeteOnlineStatus &, const KopeteOnlineStatus & ) ) );

		if ( reason.isEmpty() )
			sendInternalMessage( i18n( "%1 has left the chat." ).arg( contactName ) ) ;
		else
		{
			sendInternalMessage( i18n( "%1 has left the chat (%2)." ).
#if QT_VERSION < 0x030200
				arg( contactName ).arg( reason )
#else
				arg( contactName, reason )
#endif
			);
		}

		delete memberContactMap[ contact ];
		memberContactMap.remove( contact );
	}

	setTabState();
	emit updateStatusIcon( this );
}

void ChatView::setCaption( const QString &text, bool modified )
{
	QString newCaption = text;

	//Save this caption
	m_captionText = text;

	//Turncate if needed
	if( newCaption.length() > 20 )
		newCaption = newCaption.left( 20 ).append( QString::fromLatin1( "..." ) );

	//Call the original set caption
	KDockMainWindow::setCaption( newCaption, false );

	if( m_tabBar )
	{
		m_tabBar->setTabToolTip( this, QString::fromLatin1("<qt>%1</qt>").arg( m_captionText ) );
		m_tabBar->setTabLabel( this, newCaption  );

		//Blink icon if modified and not active
		if( !isActive && modified )
			setTabState( Changed );
		else
			setTabState();
	}

	//Tell the parent we changed our caption
	emit( captionChanged( isActive ) );
}

void ChatView::appendMessage(KopeteMessage &message)
{
	remoteTyping( message.from(), false );

	//Need to copy this because it comes in as a const
	KopeteMessage m = message;
	addChatMessage(m);
	if( !isActive )
	{
		switch (m.importance())
		{
			case KopeteMessage::Highlight:
				setTabState( Highlighted );
				break;
			case KopeteMessage::Normal:
				if(m.direction() == KopeteMessage::Inbound || m.direction() == KopeteMessage::Action)
				{
					setTabState( Message );
					break;
				}
			default:
				setTabState( Changed );
		}
	}

	if( !m_sendInProgress || message.from() != m_manager->user() )
	{
		unreadMessageFrom = message.from()->displayName();
		QTimer::singleShot( 1000, this, SLOT(slotMarkMessageRead()) );
	}
}

void ChatView::slotMarkMessageRead()
{
	unreadMessageFrom = QString::null;
}

void ChatView::slotContactStatusChanged( KopeteContact *contact, const KopeteOnlineStatus & /* newStatus */ , const KopeteOnlineStatus & /* oldstatus */)
{
	if(KopetePrefs::prefs()->showEvents() && contact)
	{
		if( contact->metaContact() )
		{
			sendInternalMessage( i18n( "%2 has changed their status to %1." )
#if QT_VERSION < 0x030200
				.arg(contact->onlineStatus().description() ).arg( contact->metaContact()->displayName() )
#else
				.arg(contact->onlineStatus().description(), contact->metaContact()->displayName() )
#endif
			);
		}
		else
		{
			sendInternalMessage( i18n( "%2 has changed their status to %1." )
#if QT_VERSION < 0x030200
				.arg( contact->onlineStatus().description() ).arg( contact->displayName() )
#else
				.arg( contact->onlineStatus().description(), contact->displayName() )
#endif
			);
		}
	}

	if(m_tabBar)
	{
		QPtrList<KopeteContact> chatMembers=msgManager()->members();
		KopeteContact *c=0L;
		for ( KopeteContact *contact = chatMembers.first(); contact; contact = chatMembers.next() )
		{
			if(!c || c->onlineStatus() < contact->onlineStatus())
				c=contact;
		}
		if(c)
			m_tabBar->setTabIconSet( this , msgManager()->contactOnlineStatus( c ).iconFor( c ) );
	}
	emit updateStatusIcon( this );
}

void ChatView::slotOpenURLRequest(const KURL &url, const KParts::URLArgs &/*args*/)
{
	kdDebug(14000) << k_funcinfo << "url=" << url.url() << endl;
	if( url.protocol() == "kopetemessage" )
	{
		KopeteContact *c = msgManager()->account()->contacts()[ url.host() ];
		if( c )
			c->execute();
	}
	else
	{
		new KRun(url, 0, false); // false = non-local files
	}
}

void ChatView::sendInternalMessage(const QString &msg)
{
	                               //when closing kopete, some internal message may be send because some contact are deleted
                                   //theses contact can already been deleted
	KopeteMessage m = KopeteMessage(/*m_manager->user(),  m_manager->members()*/ 0L,0L, msg, KopeteMessage::Internal);
	//(in many case, this is useless to set myself as contact
	//TODO: set the contact which initiate the internal messae, so we can later show a icon of it (for example, when he join a chat
	addChatMessage(m);
}

void ChatView::sendMessage()
{
	m_sendInProgress = true;
	m_mainWindow->setSendEnabled(false);

	QString txt = editpart->text( Qt::PlainText );
	if( m_lastMatch.isNull() && txt.contains( QRegExp( QString::fromLatin1("^\\w+:\\s") ) ) )
	{
		QString search = txt.left( (int)txt.find(':') );
		if( !search.isEmpty() )
		{
			QString match = mComplete->makeCompletion( search );
			if( !match.isNull() )
				m_edit->setText( txt.replace(0,search.length(),match) );
		}
	}

	if( !m_lastMatch.isNull() )
	{
		mComplete->addItem( m_lastMatch );
		m_lastMatch = QString::null;
	}

	KopeteMessage sentMessage = currentMessage();
	emit messageSent( sentMessage );
	historyList.prepend(m_edit->text());
	historyPos = -1;
	editpart->clear();
	slotStopTimer();
}

void ChatView::messageSentSuccessfully()
{
	m_sendInProgress = false;
	emit ( messageSuccess( this ) );
}

void ChatView::slotTextChanged()
{
	QString txt = m_edit->text();
	if(!editpart->simple()) //remove all <p><br> and other html tags
		txt.replace( QRegExp( QString::fromLatin1( "<[^>]*>" ) ), QString::null );

	bool typing=!txt.stripWhiteSpace().isEmpty();
	m_mainWindow->setSendEnabled( typing );

	if(typing)
	{
		// And they were previously typing
		if( !m_typingRepeatTimer->isActive() )
		{
			m_typingRepeatTimer->start( 4000, false );
			slotRepeatTimer();
		}

		// Reset the stop timer again, regardless of status
		m_typingStopTimer->start( 4500, true );
	}
}

void ChatView::historyUp()
{
	QString txt = m_edit->text();
	if(!editpart->simple()) //remove all <p><br> and other html tags
		txt.replace( QRegExp( QString::fromLatin1( "<[^>]*>" ) ), QString::null );
	bool empty=txt.stripWhiteSpace().isEmpty();

	if(historyPos == -1)
	{
		if(!empty) //if we've typed something...
		{
			historyList.prepend(m_edit->text()); //...save it
			if ((int)historyList.count() > 1)
				historyPos = 1;
			else
				historyPos = 0;
		}
		else if ((int)historyList.count() != 0)
			historyPos = 0;
	}
	else
	{
		if(!empty)
			historyList[historyPos] = m_edit->text();
		if(historyPos < (int)historyList.count()-1)
			historyPos++;
	}
	if (historyPos != -1)
	{
		m_edit->setText(historyList[historyPos]);
		m_edit->moveCursor(QTextEdit::MoveEnd, false);
	}
}

void ChatView::historyDown()
{
	QString txt = m_edit->text();
	if(!editpart->simple()) //remove all <p><br> and other html tags
		txt.replace( QRegExp( QString::fromLatin1( "<[^>]*>" ) ), QString::null );
	bool empty=txt.stripWhiteSpace().isEmpty();


	if (historyPos == -1)
	{
		if(!empty)
		{
			historyList.prepend(m_edit->text());
			m_edit->setText( "" );
		}
	}
	else
	{
		if(!empty)
			historyList[historyPos] = m_edit->text();
		historyPos--;
		if (historyPos >= 0)
		{
			m_edit->setText(historyList[historyPos]);
			m_edit->moveCursor(QTextEdit::MoveEnd, false);
		}
		else
			m_edit->setText( "" );
	}
}

void ChatView::saveOptions()
{
	KConfig *config = KGlobal::config();

	writeDockConfig ( config, QString::fromLatin1("ChatViewDock") );
	config->setGroup( QString::fromLatin1("ChatViewDock") );
	config->writeEntry( QString::fromLatin1("membersDockPosition"), membersDockPosition );
	config->setGroup( QString::fromLatin1("ChatViewSettings") );
	config->writeEntry ( QString::fromLatin1("BackgroundColor"), editpart->bgColor() );
	config->writeEntry ( QString::fromLatin1("Font"), editpart->font() );
	config->writeEntry ( QString::fromLatin1("TextColor"), editpart->fgColor() );

	config->sync();
}

void ChatView::readOptions()
{
	KConfig *config = KGlobal::config();

	/** THIS IS BROKEN !!! */
	//dockManager->readConfig ( config, "ChatViewDock" );

	//Work-around to restore dock widget positions
	config->setGroup( QString::fromLatin1("ChatViewDock") );

	membersDockPosition = static_cast<KDockWidget::DockPosition>(
		config->readNumEntry( QString::fromLatin1("membersDockPosition"), KDockWidget::DockRight ) );

	QString dockKey = QString::fromLatin1("viewDock");
	if( visibleMembers )
	{
		if( membersDockPosition == KDockWidget::DockLeft )
			dockKey.prepend( QString::fromLatin1("membersDock,") );
		else if( membersDockPosition == KDockWidget::DockRight )
			dockKey.append( QString::fromLatin1(",membersDock") );
	}
	dockKey.append( QString::fromLatin1(",editDock:sepPos") );

	int splitterPos = config->readNumEntry( dockKey, 70);
	editDock->manualDock( viewDock, KDockWidget::DockBottom, splitterPos );
	viewDock->setDockSite(KDockWidget::DockLeft | KDockWidget::DockRight );
	editDock->setEnableDocking(KDockWidget::DockNone);

	config->setGroup( QString::fromLatin1("ChatViewSettings") );

	QFont tmpFont = KGlobalSettings::generalFont();
	editpart->setFont( config->readFontEntry( QString::fromLatin1("Font"), &tmpFont) );
	QColor tmpColor = KGlobalSettings::baseColor();
	editpart->setBgColor( config->readColorEntry ( QString::fromLatin1("BackgroundColor"), &tmpColor) );
	tmpColor = KGlobalSettings::textColor();
	editpart->setFgColor( config->readColorEntry ( QString::fromLatin1("TextColor"), &tmpColor ) );
}

void ChatView::addText(const QString &text)
{
	m_edit->insert(text);
}

void ChatView::setStylesheet( const QString &style )
{
	d->xsltParser->setXSLT( style );
	slotRefreshNodes();
}

void ChatView::slotAppearanceChanged()
{
	d->xsltParser->setXSLT( KopetePrefs::prefs()->styleContents() );
	slotRefreshNodes();
}

void ChatView::addChatMessage( KopeteMessage &m )
{
	uint bufferLen = (uint)KopetePrefs::prefs()->chatViewBufferSize();

	if( transparencyEnabled )
		m.setBgOverride( bgOverride );

	messageMap.insert( ++messageId, m );
	QDomDocument message = m.asXML();
	message.documentElement().setAttribute( QString::fromLatin1("id"), QString::number(messageId) );
	QString resultHTML = addNickLinks( d->xsltParser->transform( message.toString() ) );

	QString dir = ( QApplication::reverseLayout() ? QString::fromLatin1("rtl") : QString::fromLatin1("ltr") );
	HTMLElement newNode = chatView->document().createElement( QString::fromLatin1("span") );
	newNode.setAttribute( QString::fromLatin1("dir"), dir );
	newNode.setInnerHTML( resultHTML );

	chatView->htmlDocument().body().appendChild( newNode );

	if( messageMap.count() >= bufferLen )
	{
		chatView->htmlDocument().body().removeChild( chatView->htmlDocument().body().firstChild() );
		messageMap.remove( messageMap.begin() );
	}

	if( !scrollPressed )
		QTimer::singleShot( 1, this, SLOT( slotScrollView() ) );
}

const QString ChatView::addNickLinks( const QString &html ) const
{
	QString retVal = html;

	KopeteContactPtrList members = msgManager()->members();
	for( KopeteContact *c = members.first(); c; c = members.next() )
	{
		if( c->displayName().length() > 0 )
		{
			retVal.replace(
				QRegExp( QString::fromLatin1("(?![^<]+>)(\\b%1\\b)(?![^<]+>)")
					.arg( QRegExp::escape( c->displayName() ) )  ),
				QString::fromLatin1("<a href=\"kopetemessage://%1\" class=\"KopeteDisplayName\">\\1</a>")
				.arg( c->contactId() )
			);
		}
	}

	return retVal;
}

void ChatView::slotRefreshNodes()
{
	HTMLBodyElement bodyElement = chatView->htmlDocument().body();

	QString xmlString;
	for( MessageMap::Iterator it = messageMap.begin(); it != messageMap.end(); ++it)
	{
		QDomDocument message = (*it).asXML();
	        message.documentElement().setAttribute( QString::fromLatin1("id"), QString::number( it.key() ) );
		xmlString += message.toString();
	}

	d->xsltParser->transformAsync(
		QString::fromLatin1( "<document>" ) + xmlString + QString::fromLatin1( "</document>" ),
		this, SLOT( slotTransformComplete( const QVariant & ) )
	);
}

void ChatView::slotRefreshView()
{
	HTMLElement styleElement = chatView->document().documentElement().firstChild().firstChild();
	styleElement.setInnerText( styleHTML() );

	HTMLBodyElement bodyElement = chatView->htmlDocument().body();
	bodyElement.setBgColor( KopetePrefs::prefs()->bgColor().name() );
}

void ChatView::slotTransformComplete( const QVariant &result )
{
	chatView->htmlDocument().body().setInnerHTML( addNickLinks( result.toString() ) );

	if( !scrollPressed )
		QTimer::singleShot( 1, this, SLOT( slotScrollView() ) );
}

const QString ChatView::styleHTML() const
{
	KopetePrefs *p = KopetePrefs::prefs();

	QString style = QString::fromLatin1(
		"body{margin:4px;background-color:%1;font-family:%2;font-size:%3pt;color:%4;background-repeat:no-repeat;background-attachment:fixed}"
		"td{font-family:%5;font-size:%6pt;color:%7}"
		"a{color:%8}a.visited{color:%9}"
		"a.KopeteDisplayName{text-decoration:none;color:inherit;}"
		"a.KopeteDisplayName:hover{text-decoration:underline;color:inherit}"
		".KopeteLink{cursor:pointer;}.KopeteLink:hover{text-decoration:underline}" )
		.arg( p->bgColor().name() )
		.arg( p->fontFace().family() )
		.arg( p->fontFace().pointSize() )
		.arg( p->textColor().name() )
		.arg( p->fontFace().family() )
		.arg( p->fontFace().pointSize() )
		.arg( p->textColor().name() )
		.arg( p->linkColor().name() )
		.arg( p->linkColor().name() );

	//JASON, VA TE FAIRE FOUTRE AVEC TON *default* HIGHLIGHT!
	// that broke my highlight plugin
	// if the user has not Spetialy specified that it you theses 'putaint de' default color, WE DON'T USE THEM
	if(p->highlightEnabled())
	{
		style += QString::fromLatin1( ".highlight{color:%1;background-color:%2}" )
			.arg( p->highlightForeground().name() )
			.arg( p->highlightBackground().name() );
	}

	return style;
}

void ChatView::clear()
{
	HTMLElement body = chatView->htmlDocument().body();
	while( body.hasChildNodes() )
		body.removeChild( body.childNodes().item( body.childNodes().length() - 1 ) );

	messageMap.clear();
}

KopeteContact *ChatView::contactFromNode( const Node &n ) const
{
	DOM::HTMLElement node = n;
	KopeteContact *c = 0L;

	if( !node.isNull() )
	{
		while( node.nodeType() == Node::TEXT_NODE )
			node = node.parentNode();

		if( !node.isNull() )
		{
			if( node.className() == QString::fromLatin1("KopeteDisplayName") )
			{
				KopeteContactPtrList members = msgManager()->members();
				for( c = members.first(); c; c = members.next() )
				{
					if( c->displayName() == node.innerText() )
						break;
				}
			}
		}
	}

	return c;
}

void ChatView::slotRightClick( const QString &, const QPoint &point )
{
	Node n = chatView->nodeUnderMouse();
	while( n.nodeType() == Node::TEXT_NODE )
		n = n.parentNode();

	activeElement = n;
	KopeteMessage m = messageFromNode( activeElement );
	KopeteContact *c = contactFromNode( n );

	if( c )
	{
		KPopupMenu *p = c->popupMenu( m_manager );
		connect(p,SIGNAL(aboutToHide()),p,SLOT(deleteLater()));
		p->popup( point );
	}
	else
	{
		KPopupMenu *chatWindowPopup = new KPopupMenu();

		if( activeElement.className() == QString::fromLatin1("KopeteDisplayName") )
		{
			chatWindowPopup->insertItem( i18n("User Has Left"), 1 );
			chatWindowPopup->setItemEnabled( 1, false );
			chatWindowPopup->insertSeparator();
		}
		else if( activeElement.tagName().lower() == QString::fromLatin1("a") )
		{
			copyURLAction->plug(chatWindowPopup);
			chatWindowPopup->insertSeparator();
		}

		if( !n.isNull() && msgManager()->members().contains( m.from() ) )
		{
			bool actions = false;
			int j = 0;

			QMap<KPluginInfo *, KopetePlugin *> plugins = KopetePluginManager::self()->loadedPlugins( "Plugins" );

			// Add the protocol to the list
			plugins.insert( 0L, msgManager()->protocol() );

			QMap<KPluginInfo *, KopetePlugin *>::ConstIterator it;
			for ( it = plugins.begin(); it != plugins.end(); ++it )
			{
				uint i = 0;
				QPtrList<KAction> *customActions = it.data()->customChatWindowPopupActions( m, n );
				if( customActions  && !customActions->isEmpty() )
				{
					actions = true;
					for( KAction *a = customActions->first(); a; a = customActions->next() )
						a->plug( chatWindowPopup, (++i) + j );
				}
				delete customActions;
				j++;
			}

			if( actions )
				chatWindowPopup->insertSeparator();
		}

		copyAction->setEnabled( chatView->hasSelection() );
		copyAction->plug( chatWindowPopup );
		saveAction->plug( chatWindowPopup );
		printAction->plug( chatWindowPopup );
		chatWindowPopup->insertSeparator();
		closeAction->plug( chatWindowPopup );

		connect(chatWindowPopup,SIGNAL(aboutToHide()),chatWindowPopup,SLOT(deleteLater()));
		chatWindowPopup->popup( point );
	}
}

void ChatView::slotCopyURL()
{
	HTMLAnchorElement a = activeElement;
	if( !a.isNull() )
	{
		QApplication::clipboard()->setText( a.href().string(), QClipboard::Clipboard );
		QApplication::clipboard()->setText( a.href().string(), QClipboard::Selection );
	}
}

KopeteMessage ChatView::messageFromNode( Node &node )
{
	HTMLElement e = node;

	while (!e.isNull() && e.className() != QString::fromLatin1("KopeteMessage") && e != static_cast<const DOM::Node>(chatView->htmlDocument().body()))
		e = e.parentNode();

	KopeteMessage m;
	if( e.className().string() == QString::fromLatin1("KopeteMessage") )
	{
		unsigned long mId = e.id().string().toULong();
		if( messageMap.contains( mId ) )
			m = messageMap[ mId ];
	}
	return m;
}

const QString ChatView::viewsText()
{
	return chatView->htmlDocument().body().innerHTML().string();
}

void ChatView::setActive( bool value )
{
	isActive = value;
	if( isActive )
	{
		setTabState( Normal );
		emit( activated( static_cast<KopeteView*>(this) ) );
	}
}

void ChatView::setTabBar( KTabWidget *tabBar )
{
	m_tabBar = tabBar;
}

void ChatView::refreshView()
{
	//This doesn't work well using the DOM, so just use some JS
	if( bgChanged && !backgroundFile.isNull() )
	{
		chatView->setJScriptEnabled( true ) ;
		chatView->executeScript( QString::fromLatin1("document.body.background = \"%1\";").arg( backgroundFile ) );
		chatView->setJScriptEnabled( false ) ;
	}

	bgChanged = false;

	if( !scrollPressed )
		QTimer::singleShot( 1, this, SLOT( slotScrollView() ) );
}

void ChatView::slotScrollView()
{
	htmlWidget->scrollBy( 0, htmlWidget->contentsHeight() );
}

void ChatView::setCurrentMessage( const KopeteMessage &message )
{
	m_edit->setText( message.plainBody() );

	setFont( message.font() );
	setFgColor( message.fg() );
	setBgColor( message.bg() );
}

KopeteMessage ChatView::currentMessage()
{
	KopeteMessage currentMsg = KopeteMessage( m_manager->user(), m_manager->members(), m_edit->text(), KopeteMessage::Outbound, editpart->simple() ? KopeteMessage::PlainText : KopeteMessage::RichText );

	currentMsg.setBg( editpart->bgColor() );
	currentMsg.setFg( editpart->fgColor() );
	currentMsg.setFont( editpart->font() );

	return currentMsg;
}

void ChatView::cut()
{
	m_edit->cut();
}

void ChatView::copy()
{
	if ( chatView->hasSelection() )
	{
		QApplication::clipboard()->setText( chatView->selectedText(), QClipboard::Clipboard );
		QApplication::clipboard()->setText( chatView->selectedText(), QClipboard::Selection );
	}
	else
		m_edit->copy();
}

void ChatView::paste()
{
	m_edit->paste();
}

void ChatView::print()
{
	chatView->view()->print();
}

void ChatView::selectAll()
{
	chatView->selectAll();
}

void ChatView::setBgColor( const QColor &newColor )
{
	editpart->setBgColor( newColor );
}

void ChatView::setFont()
{
	editpart->setFont();
}

void ChatView::setFont( const QFont &font )
{
	editpart->setFont( font );
}

void ChatView::setFgColor( const QColor &newColor )
{
	editpart->setFgColor( newColor );
}


void ChatView::slotRepeatTimer()
{
	emit typing( true );
}

void ChatView::slotRemoteTypingTimeout()
{
	// Remove the topmost timer from the list. Why does QPtrDict use void* keys and not typed keys? *sigh*
	if( !m_remoteTypingMap.isEmpty() )
	{
		remoteTyping( reinterpret_cast<const KopeteContact *>( QPtrDictIterator<QTimer>(m_remoteTypingMap).currentKey() ), false );
	}
}

void ChatView::slotStopTimer()
{
	m_typingRepeatTimer->stop();
	emit typing( false );
}

void ChatView::slotTransparencyChanged()
{
	transparencyEnabled = KopetePrefs::prefs()->transparencyEnabled();
	bgOverride = KopetePrefs::prefs()->bgOverride();

//	kdDebug(14000) << k_funcinfo << "transparencyEnabled=" << transparencyEnabled << ", bgOverride=" << bgOverride << "." << endl;

	if( transparencyEnabled )
	{
		if( !root )
		{
//			kdDebug(14000) << k_funcinfo << "enabling transparency" << endl;
			root = new KRootPixmap( this );
			connect(root, SIGNAL(backgroundUpdated(const QPixmap &)), this, SLOT(slotUpdateBackground(const QPixmap &)));
			root->setCustomPainting ( true );
			root->setFadeEffect(KopetePrefs::prefs()->transparencyValue() * 0.01, KopetePrefs::prefs()->transparencyColor() );
			root->start();
		} else {
			root->setFadeEffect(KopetePrefs::prefs()->transparencyValue() * 0.01, KopetePrefs::prefs()->transparencyColor() );
			root->repaint( true );
		}
	}
	else
	{
		if ( root )
		{
//			kdDebug(14000) << k_funcinfo << "disabling transparency" << endl;
			disconnect(root, SIGNAL(backgroundUpdated(const QPixmap &)), this, SLOT(slotUpdateBackground(const QPixmap &)));
			delete root;
			root = 0L;
			backgroundFile = QString::null;
			chatView->executeScript( QString::fromLatin1("document.body.background = \"\";") );
		}
	}
}

void ChatView::slotUpdateBackground(const QPixmap &pm)
{
	if( m_mainWindow )
	{
		m_mainWindow->updateBackground( pm );

		if( m_mainWindow->backgroundFile )
			backgroundFile = m_mainWindow->backgroundFile->name();

		bgChanged = true;

		refreshView();
	}
}

//-----------------------
//  KopeteContactLVI

KopeteContactLVI::KopeteContactLVI( KopeteView *view, const KopeteContact *contact, KListView *parent ) : KListViewItem( parent )
{
	m_contact = (KopeteContact*)contact;
	m_parentView = parent;
	m_view = view;

	setText( 1, QString::fromLatin1( " " ) + m_contact->displayName() );
	connect( m_contact, SIGNAL( displayNameChanged( const QString &, const QString & ) ),
		this, SLOT( slotDisplayNameChanged(const QString &, const QString &) ) );

	connect( m_contact, SIGNAL( destroyed() ), this, SLOT( deleteLater() ) );

	connect( view->msgManager(), SIGNAL( onlineStatusChanged( KopeteContact *, const KopeteOnlineStatus &, const KopeteOnlineStatus & ) ),
		this, SLOT( slotStatusChanged( KopeteContact *, const KopeteOnlineStatus &, const KopeteOnlineStatus & ) ) );

	connect( m_parentView, SIGNAL( executed( QListViewItem* ) ),
		this, SLOT( slotExecute( QListViewItem * ) ) );

	slotStatusChanged( m_contact, view->msgManager()->contactOnlineStatus(m_contact),
		view->msgManager()->contactOnlineStatus(m_contact) );
}

void KopeteContactLVI::slotDisplayNameChanged(const QString &, const QString &newName)
{
	setText( 1, QString::fromLatin1( " " ) + newName );
	m_parentView->sort();
}

void KopeteContactLVI::slotStatusChanged( KopeteContact *c, const KopeteOnlineStatus &status,
	const KopeteOnlineStatus & )
{
	if( c == m_contact )
	{
		setText( 0, QChar( -status.weight() ) );
		setPixmap( 0, status.iconFor(m_contact) );
		m_parentView->sort();
	}
}

void KopeteContactLVI::slotExecute( QListViewItem *item )
{
	if( static_cast<QListViewItem*>( this ) == item )
		((KopeteContact*)m_contact)->execute();
}


#include "chatview.moc"

// vim: set noet ts=4 sts=4 sw=4:
