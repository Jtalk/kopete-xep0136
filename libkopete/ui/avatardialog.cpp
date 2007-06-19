/*
    avatardialog.cpp - Dialog to manage and select user avatar

    Copyright (c) 2007      by Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
    Copyright (c) 2007      by Michaël Larouche       <larouche@kde.org>

    Kopete    (c) 2002-2007 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/
#include "avatardialog.h"

// KDE includes
#include <kdebug.h>
#include <klocale.h>

// Kopete includes
#include "avatarselectorwidget.h"
#include "kopeteavatarmanager.h"

namespace Kopete
{

namespace UI
{

class AvatarDialog::Private
{
public:
	Private()
	 : mainWidget(0)
	{}

	AvatarSelectorWidget *mainWidget;
	QString selectedPath;
};

AvatarDialog::AvatarDialog(QWidget *parent)
 : KDialog(parent), d(new Private)
{
	setCaption( i18n("Select an avatar") );
	setButtons( KDialog::Ok | KDialog::Cancel );

	d->mainWidget = new Kopete::UI::AvatarSelectorWidget(this);
	setMainWidget(d->mainWidget);

	connect(this, SIGNAL(okClicked()), this, SLOT(buttonOkClicked()));
}

AvatarDialog::~AvatarDialog()
{
	delete d;
}

QString AvatarDialog::selectedAvatarPath() const
{
	return d->selectedPath;
}

QString AvatarDialog::getAvatar(QWidget *parent, const QString &currentAvatar)
{
	AvatarDialog dialog(parent);
	dialog.d->mainWidget->setCurrentAvatar(currentAvatar);
	dialog.exec();

	return dialog.selectedAvatarPath();
}

void AvatarDialog::slotButtonClicked(int button)
{

	if (button == KDialog::Ok)
	{
		Kopete::AvatarManager::AvatarEntry selectedEntry = d->mainWidget->selectedEntry();

		d->selectedPath = selectedEntry.path;
	}
	else
		d->selectedPath = QString::null;

	emit result(this);

	KDialog::slotButtonClicked(button);
}

} // namespace UI

} // namespace Kopete
#include "avatardialog.moc"