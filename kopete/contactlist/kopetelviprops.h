/*
    kopetelviprops.h

    Kopete Contactlist Properties GUI for Groups and MetaContacts

    Copyright (c) 2002-2003 by Stefan Gehn            <metz AT gehn.net>
    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef KOPETELVIPROPS_H
#define KOPETELVIPROPS_H

#include <kdialogbase.h>
#include <kabc/sound.h>

#include "kopetegvipropswidget.h"
#include "kopetemetalvipropswidget.h"

class CustomNotificationProps;
class KPushButton;
class KopeteGroupViewItem;
class KopeteMetaContactLVI;
class KopeteAddressBookExport;
class KURLRequester;

class KopeteGVIProps: public KDialogBase
{
	Q_OBJECT

	public:
		KopeteGVIProps(KopeteGroupViewItem *gvi, QWidget *parent, const char *name=0L);
		~KopeteGVIProps();

	private:
		CustomNotificationProps * mNotificationProps;
		KopeteGVIPropsWidget *mainWidget;
		KopeteGroupViewItem *item;
		bool m_dirty;

	private slots:
		void slotOkClicked();
		void slotUseCustomIconsToggled(bool on);
		void slotIconChanged();
};


class KopeteMetaLVIProps: public KDialogBase
{
	Q_OBJECT

	public:
		KopeteMetaLVIProps(KopeteMetaContactLVI *gvi, QWidget *parent, const char *name=0L);
		~KopeteMetaLVIProps();

	private:
		CustomNotificationProps * mNotificationProps;
		QPushButton *mFromKABC;
		KopeteMetaLVIPropsWidget *mainWidget;
		KopeteMetaContactLVI *item;
		KopeteAddressBookExport *mExport;
		KABC::Sound mSound;

	private slots:
		void slotOkClicked();
		void slotUseCustomIconsToggled( bool on );
		void slotHasAddressbookEntryToggled( bool on );
		void slotSelectAddresseeClicked();
		void slotMergeClicked();
		void slotFromKABCClicked();
		void slotOpenSoundDialog( KURLRequester *requester );
};

#endif
