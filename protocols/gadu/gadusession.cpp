// -*- Mode: c++-mode; c-basic-offset: 2; indent-tabs-mode: t; tab-width: 2; -*-
//
// Copyright (C) 	2003	 Grzegorz Jaskiewicz <gj at pointblue.com.pl>
// Copyright (C) 	2002	 Zack Rusin <zack@kde.org>
//
// gadusession.cpp
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//

#include "gadusession.h"

#include <klocale.h>
#include <kdebug.h>

#include <qsocketnotifier.h>
#include <qtextcodec.h>
#include <qdatetime.h>

#include <netinet/in.h>
#include <errno.h>
#include <string.h>

GaduSession::GaduSession( QObject* parent, const char* name )
: QObject( parent, name ), session_( 0 ), searchSeqNr_( 0 )
{
	textcodec = QTextCodec::codecForName( "CP1250" );
}

GaduSession::~GaduSession()
{
	logoff();
}

bool
GaduSession::isConnected() const
{
	if ( session_ ) {
		return ( session_->state & GG_STATE_CONNECTED );
	}
	return false;
}

int
GaduSession::status() const
{
	kdDebug(14100)<<"Status = " << session_->status <<", initial = "<< session_->initial_status <<endl;
	if ( session_ ) {
		return session_->status;
	}
	return GG_STATUS_NOT_AVAIL;
}

void
GaduSession::login( struct gg_login_params* p )
{
	if ( !isConnected() ) {

// turn on in case you have any problems, and  you want 
// to report it better. libgadu needs to be recompiled with debug enabled
//		gg_debug_level=GG_DEBUG_MISC;	

		kdDebug(14100) << "Login" << endl;

		if ( !( session_ = gg_login( p ) ) ) {
			gg_free_session( session_ );
			session_ = 0;
			emit connectionFailed( i18n( "Internal gg_login error.Please contact author." ) );
			return;
		}

		read_ = new QSocketNotifier( session_->fd, QSocketNotifier::Read, this );
		read_->setEnabled( false );
		QObject::connect( read_, SIGNAL( activated( int ) ), SLOT( checkDescriptor() ) );

		write_ = new QSocketNotifier( session_->fd, QSocketNotifier::Write, this );
		write_->setEnabled( false );
		QObject::connect( write_, SIGNAL( activated( int ) ), SLOT( checkDescriptor() ) );
		enableNotifiers( session_->check );
		searchSeqNr_=0;
	}
}

void
GaduSession::enableNotifiers( int checkWhat )
{
	if( (checkWhat & GG_CHECK_READ) && read_ ) {
		read_->setEnabled( true );
	}
	if( (checkWhat & GG_CHECK_WRITE) && write_ ) {
		write_->setEnabled( true );
	}
}

void
GaduSession::disableNotifiers()
{
	if ( read_ ) {
		read_->setEnabled( false );
	}
	if ( write_ ) {
		write_->setEnabled( false );
	}
}

void
GaduSession::login( uin_t uin, const QString& password, bool useTls,
					int status, const QString& statusDescr )
{
	memset( &params_, 0, sizeof(params_) );

	params_.uin = uin;
	params_.password = (char *)password.ascii();
	params_.status = status;
	params_.status_descr = (char *)statusDescr.ascii();
	params_.async = 1;
	params_.tls = useTls;

	login( &params_ );
}

void
GaduSession::destroySession()
{
	if ( session_ ) {
		disableNotifiers();
		QObject::disconnect( this, SLOT(checkDescriptor()) );
		if ( read_ ) {
			delete read_;
			read_=NULL;
		}
		if ( write_ ) {
			delete write_;
			write_=NULL;
		}
		gg_free_session( session_ );
		session_ = 0;
	}
}

void
GaduSession::logoff()
{
	destroySession();
	emit disconnect();
}

int
GaduSession::notify( uin_t* userlist, int count )
{
	if ( isConnected() ) {
		return gg_notify( session_, userlist, count );
	}
	else {
		emit error( i18n("Not Connected"), i18n("You are not connected to the server!") );
	}

	return 1;
}

int
GaduSession::addNotify( uin_t uin )
{
	if ( isConnected() ) {
		return gg_add_notify( session_, uin );
	}
	else {
		emit error( i18n("Not Connected"), i18n("You are not connected to the server!") );
	}

	return 1;
}

int
GaduSession::removeNotify( uin_t uin )
{
	if ( isConnected() ) {
		gg_remove_notify( session_, uin );
	}
	else {
		emit error( i18n("Not Connected"), i18n("You are not connected to the server!") );
	}

	return 1;
}

int
GaduSession::sendMessage( uin_t recipient, const unsigned char* message, int msgClass )
{
	if ( isConnected() ) {
		return gg_send_message( session_, msgClass, recipient, message );
	}
	else {
		emit error( i18n("Not Connected"), i18n("You are not connected to the server!") );
	}

	return 1;
}

int
GaduSession::sendMessageCtcp( uin_t recipient, const QString& msg, int msgClass )
{
	if ( isConnected() ) {
		return gg_send_message_ctcp( session_, msgClass, recipient,
							reinterpret_cast<const unsigned char*>( msg.ascii() ),
							 msg.length() );
	}
	else {
		emit error( i18n("Not Connected"), i18n("You are not connected to the server!") );
	}

	return 1;
}

int
GaduSession::changeStatus( int status )
{
	kdDebug()<<"## Changing to "<<status<<endl;
	if ( isConnected() ) {
		return gg_change_status( session_, status );
	}
	else {
		emit error( i18n("Not Connected"),  i18n("You have to be connected to the server to change your status!") );
	}

	return 1;
}

int
GaduSession::changeStatusDescription( int status, const QString& descr )
{
	QString ndescr;

	ndescr= textcodec->fromUnicode(descr);

	if ( isConnected() ) {
		return gg_change_status_descr( session_, status, ndescr.ascii() );
	}
	else {
		emit error( i18n("Not Connected"), i18n("You have to be connected to the server to change your status!") );
	}

	return 1;
}

int
GaduSession::ping()
{
	if ( isConnected() ) {
		return gg_ping( session_ );
	}

	return 1;
}

int
GaduSession::dccRequest( uin_t uin )
{
	if ( isConnected() ) {
		return gg_dcc_request( session_, uin );
	}
	else {
		emit error( i18n("Not Connected"), i18n("You are not connected to the server!") );
	}

	return 1;
}

void
GaduSession::pubDirSearchClose()
{
	searchSeqNr_=0;
}

bool
GaduSession::pubDirSearch(QString& name, QString& surname, QString& nick, int UIN, QString& city,
                            int gender, int ageFrom, int ageTo, bool onlyAlive)
{
	QString bufYear;
	gg_pubdir50_t searchRequest_;

	if ( !session_ ) {
		return false;
	}

	searchRequest_ = gg_pubdir50_new( GG_PUBDIR50_SEARCH_REQUEST );
	if ( !searchRequest_ ) {
		return false;
	}

	if ( !UIN ) {
		if (name.length()) {
			gg_pubdir50_add( searchRequest_, GG_PUBDIR50_FIRSTNAME,
						(const char*)textcodec->fromUnicode( name ) );
		}
		if ( surname.length() ) {
			gg_pubdir50_add( searchRequest_, GG_PUBDIR50_LASTNAME,
						(const char*)textcodec->fromUnicode( surname ) );
		}
		if ( nick.length() ) {
			gg_pubdir50_add( searchRequest_, GG_PUBDIR50_NICKNAME,
						(const char*)textcodec->fromUnicode( nick ) );
		}
		if ( city.length() ) {
			gg_pubdir50_add( searchRequest_, GG_PUBDIR50_CITY,
						(const char*)textcodec->fromUnicode( city ) );
		}
		if ( ageFrom || ageTo ) {
			QString yearFrom = QString::number( QDate::currentDate().year() - ageFrom );
			QString yearTo = QString::number( QDate::currentDate().year() - ageTo );

			if ( ageFrom && ageTo ) {
				gg_pubdir50_add( searchRequest_, GG_PUBDIR50_BIRTHYEAR,
							(const char*)textcodec->fromUnicode( yearFrom + " " + yearTo ) );
			}
			if ( ageFrom ) {
				gg_pubdir50_add( searchRequest_, GG_PUBDIR50_BIRTHYEAR,
							(const char*)textcodec->fromUnicode( yearFrom ) );
			}
			else {
				gg_pubdir50_add( searchRequest_, GG_PUBDIR50_BIRTHYEAR,
							(const char*)textcodec->fromUnicode( yearTo ) );
			}
		}

		switch ( gender ) {
			case 1:
				gg_pubdir50_add( searchRequest_, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_MALE );
			break;
			case 2:
				gg_pubdir50_add( searchRequest_, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_FEMALE );
			break;
		}

		if ( onlyAlive ) {
			gg_pubdir50_add( searchRequest_, GG_PUBDIR50_ACTIVE, GG_PUBDIR50_ACTIVE_TRUE );
		}
	}
	// otherwise we are looking only for one fellow with this nice UIN
	else{
		gg_pubdir50_add( searchRequest_, GG_PUBDIR50_UIN, QString::number( UIN ).ascii() );
	}

	gg_pubdir50_add( searchRequest_, GG_PUBDIR50_START, QString::number(searchSeqNr_).ascii() );

	gg_pubdir50( session_, searchRequest_ );

	gg_pubdir50_free( searchRequest_ );

	return true;
}

void
GaduSession::sendResult( gg_pubdir50_t result )
{
	int i, count, age;
	resLine *rl = NULL;
	searchResult sres;

	count = gg_pubdir50_count( result );

	sres.setAutoDelete( TRUE );

	for ( i = 0; i < count; i++ ) {
		rl = new resLine;

		rl->uin		= textcodec->toUnicode( gg_pubdir50_get( result, i, GG_PUBDIR50_UIN ) );
		rl->firstname	= textcodec->toUnicode( gg_pubdir50_get( result, i, GG_PUBDIR50_FIRSTNAME ));
		rl->surname	= textcodec->toUnicode( gg_pubdir50_get( result, i, GG_PUBDIR50_LASTNAME ));
		rl->nickname	= textcodec->toUnicode( gg_pubdir50_get(result, i, GG_PUBDIR50_NICKNAME ) );
		rl->age		= textcodec->toUnicode( gg_pubdir50_get( result, i, GG_PUBDIR50_BIRTHYEAR ) );
		rl->city		= textcodec->toUnicode( gg_pubdir50_get( result, i, GG_PUBDIR50_CITY ) );
		QString stat	= textcodec->toUnicode( gg_pubdir50_get( result, i, GG_PUBDIR50_STATUS ) );
		rl->status		= stat.toInt();
		age = rl->age.toInt();
		if ( age ) {
			rl->age = QString::number( QDate::currentDate().year() - age );
		}
		else {
			rl->age.truncate( 0 );
		}
		sres.append( rl );
		kdDebug(14100) << "found line "<< rl->uin << " " << rl->firstname << endl;
	}

	searchSeqNr_ = gg_pubdir50_next( result );

	emit pubDirSearchResult( sres );
}

void
GaduSession::requestContacts()
{
	if ( !session_ || session_->state != GG_STATE_CONNECTED ) {
		kdDebug(14100) <<" you need to be connected to send " << endl;
		return;
	}

	if ( gg_userlist_request( session_, GG_USERLIST_GET, NULL ) == -1 ) {
		kdDebug(14100) <<" userlist export ERROR " << endl;
		return;
	}
	kdDebug( 14100 ) << "Contacts list import..started " << endl;
}

void
GaduSession::exportContacts( gaduContactsList* u )
{
	QPtrListIterator<contactLine>loo( *u );
	unsigned int i;
	QString plist, contacts;

	if ( !session_ || session_->state != GG_STATE_CONNECTED ) {
		kdDebug( 14100 ) << "you need to connect to export Contacts list " << endl;
		return;
	}

	for ( i=u->count() ; i-- ; ++loo ) {
//	name;surname;nick;displayname;telephone;group(s);uin;email;0;;0;
		contacts +=
			(*loo)->firstname+";"+(*loo)->surname+";"+(*loo)->nickname+";"+(*loo)->name+";"+
			(*loo)->phonenr+";"+(*loo)->group+";"+(*loo)->uin+";"+(*loo)->email+";0;;0;\n";
	}

	// FIXME:Remove before release
	kdDebug(14100) <<"--------------------userlists\n" << contacts << "\n---------------" << endl;

	plist = textcodec->fromUnicode( contacts );

	if ( gg_userlist_request( session_, GG_USERLIST_PUT, plist.ascii() ) == -1 ) {
		kdDebug( 14100 ) << "export contact list failed " << endl;
		return;
	}
	kdDebug( 14100 ) << "Contacts list export..started " << endl;
}


bool
GaduSession::stringToContacts( gaduContactsList& gaducontactslist , const QString& sList )
{
	QString cline;
	QStringList::iterator it;
	QStringList strList ;
	contactLine* cl = NULL;
	bool email = false;

	if ( sList.isEmpty() || sList.isNull() || !sList.contains( ';' ) ) {
		return false;
	}

	if ( !sList.contains( '\n' ) && sList.contains( ';' ) ) {
		// basicaly, server stores contact list as it is
		// even if i will send him windows 2000 readme file
		// he will accept it, and he will return it on each contact import
		// so, if you have any problems with contact list
		// this is probably not my fault, only previous client issue
		// iow: i am not bothered :-)
		kdDebug(14100)<<"you have to retype your contacts, and export list again"<<endl;
		kdDebug(14100)<<"send this line to author, if you think it should work"<<endl;
		kdDebug(14100)<<"------------------------------------------------------"<<endl;
		kdDebug(14100)<<"\"" << sList << "\""<<endl;
		kdDebug(14100)<<"------------------------------------------------------"<<endl;
		return false;
	}

	QStringList ln  = QStringList::split( QChar( '\n' ),  sList, true );
	QStringList::iterator lni = ln.begin( );

	while( lni != ln.end() ) {
		cline = (*lni);
		if ( cline.isNull() ) {
			break;
		}
		kdDebug(14100)<<"\""<< cline << "\"" << endl;

		strList  = QStringList::split( QChar( ';' ), cline, true );
		if ( strList.count() == 12 ) {
			email=true;
		}

		if ( strList.count() != 8 && email == false ) {
			kdDebug(14100)<< "fishy, maybe contact format was changed. Contact author/update software"<<endl;
			kdDebug(14100)<<"nr of elements should be 8 or 12, but is "<<strList.count() << " LINE:" << cline <<endl;
			++lni;
			continue;
		}

		it = strList.begin();
//each line ((firstname);(secondname);(nickname).;(name);(tel);(group);(uin);

		if ( cl == NULL ) {
			cl = new contactLine;
		}

		cl->firstname	= (*it);
		cl->surname	= (*++it);
		cl->nickname	= (*++it);
		cl->name		= (*++it);
		cl->phonenr	= (*++it);
		cl->group		= (*++it);
		cl->uin		= (*++it);
		if ( email ) {
			cl->email	= (*++it);
        	}
		else {
			 cl->email	= "";
		}

		++lni;

		if ( cl->uin.isNull() ) {
			kdDebug(14100) << "no Uin, strange "<<endl;
			kdDebug(14100) << "LINE:" << cline <<endl;
			// FIXME: maybe i should consider this as an fatal error, and return false
			continue;
		}

		gaducontactslist.append( cl );
		kdDebug(14100) << "adding to list ,uin:" << cl->uin <<endl;

		cl = NULL;
	}
	return true;
}

void
GaduSession::handleUserlist( gg_event* e )
{
	QString ul;
	switch( e->event.userlist.type ) {
		case GG_USERLIST_GET_REPLY:
			if ( e->event.userlist.reply ) {
				ul = e->event.userlist.reply;
				kdDebug( 14100 ) << "Got Contacts list  OK " << endl;
			}
			else {
				kdDebug( 14100 ) << "Got Contacts list  FAILED/EMPTY " << endl;
				// FIXME: send failed?
			}
			emit userListRecieved( ul );
			break;

		case GG_USERLIST_PUT_REPLY:
			kdDebug( 14100 ) << "Contacts list exported  OK " << endl;
			emit userListExported();
			break;

	}
}

const
QString
GaduSession::failureDescription( gg_failure_t f )
{
	switch( f ) {
		case GG_FAILURE_RESOLVING:
			return i18n( "Unable to resolve server address. DNS failure." );
			break;
		case GG_FAILURE_CONNECTING:
			return i18n( "Unable to connect to server." );
			break;
		case GG_FAILURE_INVALID:
			return i18n( "Server send incorrect data. Protocol error." );
			break;
		case GG_FAILURE_READING:
			return i18n( "Problem reading data from server." );
			break;
		case GG_FAILURE_WRITING:
			return i18n( "Problem sending data to server." );
			break;
		case GG_FAILURE_PASSWORD:
			return i18n( "Incorrect password." );
			break;
		case GG_FAILURE_404:
			return QString::fromAscii( "404." );
			break;
		case GG_FAILURE_TLS:
			return i18n( "Unable to connect over encrypted channel.\nTry to turn off encryption support in Gadu account settings and reconnect." );
			break;
	}
	
	return i18n( "Unknown error number %1." ).arg( QString::number( (unsigned int)f ) );	
}

void
GaduSession::checkDescriptor()
{
	disableNotifiers();

	struct gg_event *e;

	if ( !( e = gg_watch_fd( session_ ) ) ) {
		kdDebug(14100)<<"Connection was broken for some reason"<<endl;
		logoff();
		return;
	}

	switch( e->type ) {
		case GG_EVENT_MSG:
			if ( e->event.msg.msgclass == GG_CLASS_CTCP ) {
				// TODO: DCC CONNECTION
			}
			else {
				emit messageReceived( e );
			}
		break;
		case GG_EVENT_ACK:
			emit ackReceived( e );
		break;
		case GG_EVENT_STATUS:
			kdDebug(14100)<<"event status < 60, i should not get it boy"<<endl;
		break;
		case GG_EVENT_STATUS60:
			emit statusChanged( e );
		break;
		case GG_EVENT_NOTIFY60:
			emit notify( e );
		break;
		case GG_EVENT_CONN_SUCCESS:
			emit connectionSucceed( e );
		break;
		case GG_EVENT_CONN_FAILED:
			kdDebug(14100) << "event connectionFailed" << endl;
			destroySession();
			if ( e->event.failure == GG_FAILURE_PASSWORD ) {
				kdDebug(14100) << "incorrect passwd, retrying" << endl;
				emit loginPasswordIncorrect();
				break;
			}
			if ( e->event.failure == GG_FAILURE_TLS ) {
				emit tlsConnectionFailed();
				break;
			}
			kdDebug(14100) << "emit connection failed signal" << endl;
			emit connectionFailed( failureDescription( (gg_failure_t)e->event.failure ) );
			
			break;
		case GG_EVENT_DISCONNECT:
			kdDebug(14100)<<"event Disconnected"<<endl;
			logoff();
		break;
		case GG_EVENT_PONG:
			emit pong();
		break;
		case GG_EVENT_NONE:
			break;
		case GG_EVENT_PUBDIR50_SEARCH_REPLY:
		case GG_EVENT_PUBDIR50_WRITE:
		case GG_EVENT_PUBDIR50_READ:
			sendResult( e->event.pubdir50 );
	        break;
		case GG_EVENT_USERLIST:
			handleUserlist( e );
		break;
		default:
			kdDebug(14100)<<"Unprocessed GaduGadu Event = "<<e->type<<endl;
		break;
	}

	if ( e ) {
		gg_free_event( e );
	}

	if ( session_ ) {
		enableNotifiers( session_->check );
	}
}

#include "gadusession.moc"
