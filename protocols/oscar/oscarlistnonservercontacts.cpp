// oscarlistnonservercontacts.cpp

// Copyright (C)  2005  Matt Rogers <mattr@kde.org>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA
// 02110-1301, USA.


#include "oscarlistnonservercontacts.h"
#include "oscarlistcontactsbase.h"
#include <qstringlist.h>
#include <klocale.h>

OscarListNonServerContacts::OscarListNonServerContacts(QWidget* parent)
    : KDialogBase( parent, 0, false, i18n( "Add contacts to server list" ),
                   Ok | Cancel )
{
    m_contactsList = new OscarListContactsBase( this );
    setMainWidget( m_contactsList );
    setButtonText( Ok, i18n( "&Add" ) );
    setButtonText( Cancel, i18n( "Do &not add" ) );

}

OscarListNonServerContacts::~OscarListNonServerContacts()
{

}

void OscarListNonServerContacts::addContacts( const QStringList& contactList )
{
    m_nonServerContacts = contactList;
    m_contactsList->notServerContacts->insertStringList( contactList );
}

QStringList OscarListNonServerContacts::nonServerContactList() const
{
    return m_nonServerContacts;
}

#include "oscarlistnonservercontacts.moc"
