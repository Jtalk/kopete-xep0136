/***************************************************************************
                          msnprotocol.h  -  description
                             -------------------
    begin                : Wed Jan 2 2002
    copyright            : (C) 2002 by duncan
    email                : duncan@tarro
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MSNPROTOCOL_H
#define MSNPROTOCOL_H

#include <qpixmap.h>
#include <qwidget.h>
#include "msnpreferences.h"
#include <statusbaricon.h>
#include <addcontactpage.h>
#include <improtocol.h>


/**
  *@author duncan
  */

class MSNProtocol : public QObject, public IMProtocol
{
Q_OBJECT
public: 
	MSNProtocol();
	~MSNProtocol();
	/* Plugin reimplementation */
	void init();
	bool unload();
	/** IMProtocol reimplementation */
	virtual QPixmap getProtocolIcon();
	virtual AddContactPage *getAddContactWidget(QWidget *parent);
	virtual void Connect();
	virtual bool isConnected();
	/** Internal */
	StatusBarIcon *statusBarIcon;
private:
	void initIcons();
	QPixmap protocolIcon;
	QPixmap onlineIcon;
	QPixmap offlineIcon;
};

#endif
