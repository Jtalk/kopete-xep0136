/*
    eventtransfer.h - Kopete Groupwise Protocol
    
    Copyright (c) 2004      SUSE Linux AG	 	 http://www.suse.com
    
    Kopete (c) 2002-2004 by the Kopete developers <kopete-devel@kde.org>
 
    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef GW_EVENTTRANSFER_H
#define GW_EVENTTRANSFER_H

#include <q3cstring.h>
#include <qdatetime.h>

#include "gwerror.h" 

#include "transfer.h"

namespace Event {
	
}

/**
 * Transfer representing an event, a message generated by the server in response to external stimulus
 * This class can contain varying data items depending on the type of event.  
 * You can query which data is present before trying to access it
 * @author Kopete Developers
 */
class EventTransfer : public Transfer
{
public:
	/** 
	 * Flags describing the possible contents of an event transfer
	 */
	enum Contents { EventType = 	0x00000001, 
					Source = 		0x00000002, 
					TimeStamp = 	0x00000004, 
					Guid = 			0x00000008,
					Flags = 		0x00000010,
					Message = 		0x00000020,
					Status = 		0x00000040,
					StatusText = 	0x00000080 };
	/**
	 * Constructor
	 * @param eventType the event code describing the event, see @refGroupWise::Event.
	 * @param source the user generating the event.
	 * @param timeStamp the time at which the event was received.
	 */
	EventTransfer( const quint32 eventType, const QString & source, QDateTime timeStamp );
	~EventTransfer();
	/** 
	 * Access the bitmask that describes the transfer's contents.  Use @ref Contents to determine what it contains
	 */
	quint32 contents();
	/**
	 * Convenience accessors to see what the transfer contains
	 */
	bool hasEventType();
	bool hasSource();
	bool hasTimeStamp();
	bool hasGuid();
	bool hasFlags();
	bool hasMessage();
	bool hasStatus();
	bool hasStatusText();
	
	/**
	 * Accessors for the transfer's contents
	 */
	TransferType type() { return Transfer::EventTransfer; }
	int eventType();
	QString source();
	QDateTime timeStamp();
	GroupWise::ConferenceGuid guid();
	quint32 flags();
	QString message();
	quint16 status();
	QString statusText();
	
	/**
	 * Mutators to set the transfer's contents
	 */
	void setGuid( const GroupWise::ConferenceGuid & guid );
	void setFlags( const quint32 flags );
	void setMessage( const QString & message );
	void setStatus( const quint16 status );
	void setStatusText( const QString & statusText);
	
private:
	quint32 m_contentFlags;
	int m_eventType;
	QString m_source;
	QDateTime m_timeStamp;
	GroupWise::ConferenceGuid m_guid;
	quint32 m_flags;
	QString m_message;
	quint16 m_status;
	QString m_statusText;
};

#endif
