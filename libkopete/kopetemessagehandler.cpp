/*
    kopetemessagefilter.cpp - Kopete Message Filtering

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

#include "kopetemessagehandler.h"
#include "kopetemessageevent.h"

namespace Kopete
{

class MessageHandler::Private
{
public:
	Private() : next(0) {}
	MessageHandler *next;
};

MessageHandler::MessageHandler()
 : QObject( 0 ), d( new Private )
{
}

MessageHandler::~MessageHandler()
{
	delete d;
}

MessageHandler *MessageHandler::next()
{
	return d->next;
}

void MessageHandler::setNext( MessageHandler *next )
{
	d->next = next;
}

int MessageHandler::capabilities()
{
	return d->next->capabilities();
}

void MessageHandler::handleMessage( MessageEvent *event )
{
	d->next->handleMessage(event);
}


class MessageHandlerFactory::Private
{
public:
	static FactoryList factories;
	FactoryList::Iterator iterator;
};
MessageHandlerFactory::FactoryList MessageHandlerFactory::Private::factories;

MessageHandlerFactory::MessageHandlerFactory()
	: d( new Private )
{
	d->iterator = Private::factories.append(this);
}

MessageHandlerFactory::~MessageHandlerFactory()
{
	Private::factories.remove( d->iterator );
	delete d;
}

MessageHandlerFactory::FactoryList MessageHandlerFactory::messageHandlerFactories()
{
	return Private::factories;
}

}

#include "kopetemessagehandler.moc"

// vim: set noet ts=4 sts=4 sw=4:
