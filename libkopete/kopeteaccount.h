/*
    kopeteaccount.h - Kopete Account

    Copyright (c) 2003-2004 by Olivier Goffart       <ogoffart@tiscalinet.be>
    Copyright (c) 2003-2004 by Martijn Klingens      <klingens@kde.org>
    Copyright (c) 2004      by Richard Smith         <kde@metafoo.co.uk>
    Kopete    (c) 2002-2004 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef KOPETEACCOUNT_H
#define KOPETEACCOUNT_H

#include "qobject.h"
#include <kdemacros.h>
#include <qdict.h>

class QDomNode;
class KActionMenu;
class KConfigGroup;

struct KopeteAccountPrivate;

namespace Kopete
{

class Contact;
class Plugin;
class Protocol;
class MetaContact;
class OnlineStatus;
class BlackLister;
class Group;

/**
 * @author Olivier Goffart  <ogoffart@tiscalinet.be>
 *
 * The Kopete::Account class handles one account.
 * Each protocol should subclass this class in their own custom accounts class.
 * There are few pure virtual method that the protocol must implement. Examples are:
 * \li \ref connect()
 * \li \ref disconnect()
 * \li \ref createContact()
 *
 * The accountId is an <b>constant</b> unique id, which represents the login.
 * The @ref myself() contact is one of the most important contacts, which represents
 * the user tied to this account. You must create this contact in the contructor of your
 * account and use @ref setMyself()
 *
 * All account data is automatically saved to @ref KConfig. This includes the
 * accountId, the password (in a encrypted format), the autoconnect flag, the color,
 * and all PluginData. Kopete::Account is a @ref Kopete::PluginDataObject. In the account,
 * you can call setPluginData( protocol() , "key" , "value") and pluginData( protocol , "key")
 * see @ref Kopete::PluginDataObject::setPluginData() and \ref Kopete::PluginDataObject::pluginData()
 * Note that plugin data are not availiable in the account constructor. They are
 * only available after the XML file has been totally parsed. You can reimplement
 * @ref Kopete::Account::loaded() to do what you have to do right after the XML file is
 * loaded. in the same way, you can't set pluginData in the destructor, because the
 * XML file has already been written, and new changes will not be updated on the disk.
 */
class Account : public QObject
{
	Q_OBJECT

	Q_PROPERTY( QString accountId READ accountId )
	Q_PROPERTY( QString password READ password WRITE setPassword )
	Q_PROPERTY( bool rememberPassword READ rememberPassword )
	Q_PROPERTY( bool autoConnect READ autoConnect WRITE setAutoConnect )
	Q_PROPERTY( QColor color READ color WRITE setColor )
	Q_PROPERTY( QPixmap accountIcon READ accountIcon )
	Q_PROPERTY( bool isConnected READ isConnected )
	Q_PROPERTY( bool isAway READ isAway )
	Q_PROPERTY( bool suppressStatusNotification READ suppressStatusNotification )
	Q_PROPERTY( uint priority READ priority WRITE setPriority )

public:
	/**
	 * \brief Describes what should be done when the contact is added to a metacontact
	 */
	enum AddMode { ChangeKABC=0, DontChangeKABC=1 ,  Temporary=2  };

	/**
	 * \brief Describes how the account was disconnected
	 *
	 * Manual means that the disconnection was done by the user and no reconnection
	 * will take place. Any other value will reconnect the account on disconnection.
	 * The case where the password is wrong will be handled differently.
	 */
	enum DisconnectReason {
		BadUserName = -2,
		InvalidHost = -1,
		Manual = 0,
		ConnectionReset = 1,
		Unknown = 99 };

	/**
	 * constructor:
	 * The constructor register automatically the account to the @ref Kopete::AccountManager
	 * @param parent it the protocol of this account. the accoun is a child object of the
	 * protocol, so it will be automatically deleted with the parent.
	 * @param accountID is the id of this protocol, it shouln't be changed after
	 * @param name is the QObject name. it can be 0L
	 */
	Account(Protocol *parent, const QString &accountID, const char *name=0L);
	~Account();

	/**
	 * \return the Kopete::Protocol for this account
	 */
	Protocol *protocol() const ;

	/**
	 * \return the unique id of this account used as the login
	 */
	QString accountId() const;

	/**
	 * The account ID should be constant, don't use this method.
	 * Reserved for expert usage, use it with care of risk
	 * Changing the accountId on the fly, can result in bug and crash.
	 * the myself contact id will never be changed (it is impossible to
	 * change the contactId on the fly, and deleting myself is not possible
	 * because it is connected to slot, pointer to this contact are stored
	 * everywere, for examples, in the KMM.
	 */
	void setAccountId( const QString &accountId );

 	/**
	 * \brief Get the priority of this account.
	 *
	 * Used for sorting and determining the preferred account to message a contact
	 */
	const uint priority() const;

	/**
	 * \brief Set the priority of this account.
	 *
	 * This method is called by the UI, and should not be called elsewhere.
	 */
	void setPriority( uint priority );

	/**
	 * \brief Get the password for this account.
	 *
	 * The user will be prompted if for a password if the password is not currently set
	 * @param error Set this value to true if you previously called password and the
	 * result was incorrect (the password was wrong). It adds a label in the input
	 * dialog saying that the password was wrong
	 * @param ok is set to false if the user returned cancel
	 * @param maxLength maximum length for a password, restricts the password
	 * lineedit accordingly, the default is no limit at all
	 * @return The password or QString::null if the user has canceled
	 */
	QString password( bool error = false, bool *ok =0L, unsigned int maxLength=0 ) const;

	/**
	 * \brief Set the password for this account.
	 * @param pass contains the new password. If set to QString::null, the password
	 * is forgotten.
	 *
	 * Should only be called by EditAccountWidget
	 */
	void setPassword(const QString &pass = QString::null);

	/**
	 * @brief Get if the password is remembered.
	 *
	 * This function should be used by the EditAccountPage (and only by the EditAccountPage)
	 *
	 * \return true if the password is remembered
	 * \return false if the user needs to be prompted for a password
	 */
	bool rememberPassword() const;

	/**
	 * \brief Set if the account should log in automatically.
	 *
	 * This function can be used by the EditAccountPage (and only by it)
	 * Kopete handles the autoconnection automatically
	 */
	void setAutoConnect(bool);
	/**
	 * \brief Get if the account should log in automatically.
	 *
 	 * This function can be used by the EditAccountPage (and only by it)
	 * Kopete handles the autoconnection automatically
	 */
	bool autoConnect() const;

	/**
	 * \brief Get the color for this account.
	 * \return the user color for this account
	 */
	const QColor color() const;

	/**
	 * \brief Set the color for this account.
	 *
	 * The color will be used to differentiate this account from the other accounts
	 * Normally, this should be called by Kopete's account config page so you
	 * don't have to set the color yourself
	 */
	void setColor( const QColor &color);

	/**
	 * \brief Get the icon for this account.
	 *
	 * The icon is not cached.
	 * \return the icon for this account, colored if needed
	 * @param size is the size of the icon.  If the size is 0, the default size is used
	 *
	 */
	QPixmap accountIcon(const int size=0) const;

	/**
	 * @brief Indicate whether the account is connected at all.
	 *
	 * This is a convenience method that queries @ref Kopete::Contact::onlineStatus()
	 * on @ref myself()
	 */
	bool isConnected() const;

	/**
	 * @brief Indicate whether the account is away.
	 *
	 * This is a convenience method that queries @ref Kopete::Contact::onlineStatus()
	 * on @ref myself()
	 */
	bool isAway() const;

	/**
	 * \brief Retrieve the 'myself' contact.
	 *
	 * \return the pointer to the Kopete::Contact object for this
	 * account or NULL if not initialized
	 *
	 * \see setMyself().
	 */
	Contact * myself() const;

	/**
	 * @brief Return the menu for this account
	 *
	 * You have to reimplement this method to return the custom action menu which will
	 * be shown in the statusbar. Kopete takes care of the deletion of the menu. Actions
	 * should have the menu as parent.
	 */
	virtual KActionMenu* actionMenu() ;

	/**
	 * @brief Retrieve the list of contacts for this protocol
	 *
	 * The list is guaranteed to contain only contacts for this account,
	 * so you can safely use static_cast to your own derived contact class
	 * if needed.
	 */
	const QDict<Contact>& contacts();

	/**
	 * \internal
	 * Register a new Kopete::Contact with the account
	 * To be called <b>only</b> from @ref Kopete::Contact constructor
	 * not from any other class! (Not even a derived class).
	 */
	void registerContact( Contact *c );


	/**
	  * Return the @ref KConfigGroup used to write and read special properties
	  *
	  * "Protocol", "AccountId" , "Color", "AutoConnect", "Priority", "Enabled" are reserved keyword already in use in that group
	  */
	KConfigGroup *configGroup() const;

	/**
	 * Indicates whether or not we should suppress status notifications
	 * for contacts belonging to this account.
	 * \return true if notifications should not be used, false if
	 * notifications should be used
	 */
	bool suppressStatusNotification() const;

	/**
	 * \return a pointer to the blacklist of the account
	 */
	BlackLister* blackLister();

	/**
	 * \return @c true if the contact with ID @p contactId is in the blacklist, @c false otherwise
	 */
	virtual bool isBlocked( const QString &contactId );

protected:
	/**
	 * \brief Set the 'myself' contact.
	 *
	 * This contact @em must be defined for every  account, because it
	 * holds the online status of an account!
	 * You must call this function in the constructor of your account.
	 *
	 * The @p myself contact can't be deleted as long as the account still
	 * exists. The @p myself contact is used in each Kopete::MessageManager,
	 * the @p myself contactId should be the accountID, the onlineStatus
	 * should represent the current user's status. The statusbar icon
	 * is connected to @p myself's @ref Kopete::Contact::onlineStatusChanged()
	 * to update the icon.
	 */
	void setMyself( Contact *myself );

	/**
	 * \brief Create a new contact in the specified metacontact
	 *
	 * You shouldn't ever call this method yourself, For adding contacts see @ref addContact()
	 *
	 * This method is called by @ref Kopete::Account::addContact() in this method, you should
	 * simply create the new custom @ref Kopete::Contact in the given metacontact. And you can
	 * add the contact to the server if the protocol supports it
	 *
	 * @param contactId The unique ID for this protocol
	 * @param parentContact The metacontact to add this contact to
	 */
	virtual bool createContact( const QString &contactId, MetaContact *parentContact ) =0;

public slots:
	/**
	 * @brief Go online for this service.
	 *
	 */
	virtual void connect() = 0;

	/**
	 * @brief Go online for this service using a different status
	 */
	virtual void connect( const OnlineStatus& initialStatus );

	/**
	 * @brief Disconnect from this service.
	 * \deprecated
	 */
	virtual void disconnect() = 0 ;

	/**
	 * \brief Disconnect from this service with the specified reason
	 *
	 * \todo Merge this method and \ref disconnect when we decide to
	 * break binary compatibility
	 */
	virtual void disconnect( DisconnectReason reason );


	/**
	 * \brief add a contact to a new metacontact
	 *
	 * This method will add a new metacontact containing only the contact.
	 * It will take care the contact isn't already on the contactlist.
	 * if it is already, the contact is not created, but the metacontact containing the contact is returned
	 *
	 * @param contactId The @ref Contact::contactId
	 * @param displayName The displayname (alias) of the new metacontact. Let empty if not applicable.
	 * @param group the group to add the contact. if NULL, it will be added to toplevel
	 * @param mode If the KDE addressbook should be changed to include the new contact. Don't change if you are using this method to deserialise.  
	 *   if Temporary, @p group is not used
	 * @return the new metacontact or 0L if no contact was created because of error.
	 */
	MetaContact *addMetaContact( const QString &contactId, const QString &displayName = QString::null, Group *group=0L, AddMode mode = DontChangeKABC ) ;

	/**
	 * @brief add a contact to an existing metacontact
	 *
	 * This method will check if the contact is not already in the contactlist, and if not, will add a contact
	 * to the given metacontact.
	 *
	 * @param contactId The @ref Contact::contactId of the contact
	 * @param parent The metaContact parent (must exists)
	 * @param mode If the KDE address book should be updated. see @ref AddMode.  
	 *
	 */
	bool addContact(const QString &contactId , MetaContact *parent, AddMode mode = DontChangeKABC );

	/**
	 * this will be called if main-kopete wants
	 * the plugin to set the user's mode to away
	 */
	virtual void setAway( bool away, const QString &reason = QString::null ) = 0;

	/**
	 * Add a user to the blacklist. The default implementation calls
	 * blackList()->addContact( contactId )
	 *
	 * @param contactId the contact to be added to the blacklist
	 */
	virtual void block( const QString &contactId );

	/**
	 * Remove a user from the blacklist. The default implementation calls
	 * blackList()->removeContact( contactId )
	 *
	 * @param contactId the contact to be removed from the blacklist
	 */
	virtual void unblock( const QString &contactId );


	/**
	 * @deprecated   place everithing in the constructor
	 * @todo remove
	 */
	virtual void loaded() {};

signals:
	/**
	 * The accountId should be constant, see @ref Kopete::Account::setAccountId()
	 */
	void accountIdChanged();

	/**
	 * The color of the account has been changed
	 */
	void colorChanged( const QColor & );

	void accountDestroyed( const Kopete::Account* );

private slots:
	/**
	 * Track the deletion of a Kopete::Contact and cleanup
	 */
	void slotKopeteContactDestroyed( Kopete::Contact * );

	/**
	 * Our online status changed.
	 *
	 * Currently this slot is used to set a timer that allows suppressing online status
	 * notifications just after connecting.
	 */
	void slotOnlineStatusChanged( Kopete::Contact *contact, const Kopete::OnlineStatus &newStatus, const Kopete::OnlineStatus &oldStatus );

	/**
	 * Stop the suppression of status notification
	 */
	void slotStopSuppression();

private:

	KopeteAccountPrivate *d;



public:
	/**
	 * @todo remove
	 * @deprecated  uses configGroup
	 */
	void setPluginData( Plugin *plugin, const QString &key, const QString &value ) KDE_DEPRECATED;

	/**
	 * @todo remove
	 * @deprecated  uses configGroup
	 */
	QString pluginData( Plugin *plugin, const QString &key ) const KDE_DEPRECATED;

};

}

#endif

// vim: set noet ts=4 sts=4 sw=4:

