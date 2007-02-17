/*
    kircentity.cpp - IRC Client

    Copyright (c) 2004-2007 by Michel Hermier <michel.hermier@gmail.com>

    Kopete    (c) 2004-2007 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "kircentity.moc"

#include "kircentitymanager.h"

#include <kdebug.h>

using namespace KIrc;

/**
 * Match a possible user definition:
 * nick!user@host
 * where user and host are optional.
 * NOTE: If changes are done to the regexp string, update also the sm_userStrictRegExp regexp string.
 */
//const QRegExp Entity::sm_userRegExp(QLatin1String("^([^\\s,:!@]+)(?:(?:!([^\\s,:!@]+))?(?:@([^\\s,!@]+)))?$"));

/**
 * Regexp to match strictly the complete user definition:
 * nick!user@host
 * NOTE: If changes are done to the regexp string, update also the sm_userRegExp regexp string.
 */
//const QRegExp Entity::sm_userStrictRegExp(QLatin1String("^([^\\s,:!@]+)!([^\\s,:!@]+)@([^\\s,:!@]+)$"));

//const QRegExp Entity::sm_channelRegExp(QLatin1String("^[#!+&][^\\s,]+$") );
/*
// FIXME: Implement me
EntityType Entity::guessType(const QByteArray &)
{
	return Unknown;
}

bool Entity::isUser( const QByteArray &name )
{
	return sm_userRegExp.exactMatch(name);
}

bool Entity::isChannel( const QByteArray &name )
{
	return sm_channelRegExp.exactMatch(name);
}
*/
class KIrc::Entity::Private
{
public:
	Private()
		: type(Unknown)
		, codec(0)
	{ }

	Entity::Type type;

	QByteArray name;
	QByteArray host;

	QByteArray awayMessage;
	QByteArray modes;
	QByteArray topic;

	QTextCodec *codec;
};

Entity::Entity(QObject *parent)
	: QObject(parent)
	, d(new Private)
{
}

Entity::~Entity()
{
	delete d;
}

Entity::Type Entity::type() const
{
	return d->type;
}

QByteArray Entity::topic() const
{
	return d->topic;
}

bool Entity::isChannel() const
{
	return type() == Channel;
}

bool Entity::isUser() const
{
	return type() == User;
}

void Entity::setType( Entity::Type type )
{
	if ( d->type != type )
	{
//		d->status.type = type;
		emit updated();
	}
}
/*
EntityType Entity::guessType()
{
	setType( guessType(d->name) );
	return type();
}
*/
QByteArray Entity::name() const
{
	return d->name;
}

void Entity::setName(const QByteArray &name)
{
	if ( d->name != name )
	{
		d->name = name;
		emit updated();
	}
}

QByteArray Entity::host() const
{
	return d->host;
}

void Entity::setAwayMessage(const QByteArray &awayMessage)
{
	if ( d->awayMessage != awayMessage )
	{
		d->awayMessage = awayMessage;
		emit updated();
	}
}

QByteArray Entity::modes() const
{
	return d->modes;
}

QByteArray Entity::setModes(const QByteArray &modes)
{
#ifdef __GNUC__
	#warning this needs more logic to handle the +/- modes.
#endif
	if ( d->modes != modes )
	{
		d->modes = modes;
		emit updated();
	}
	return d->modes;
}

QTextCodec *Entity::codec() const
{
	return d->codec;
}

void Entity::setCodec(QTextCodec *codec)
{
	if ( d->codec != codec )
	{
		d->codec = codec;
		emit updated();
	}
}
