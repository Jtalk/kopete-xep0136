#ifndef JT_ARCHIVE_PREFERENCES_H
#define JT_ARCHIVE_PREFERENCES_H

#include <QtCore/QString>
#include <QtXml/QDomElement>


namespace JT_Archive_Helper {
/**
 * \brief The Preferences struct contains XEP-0136 preferences representation,
 * as well as string-to-enum and vice versa conversion routines.
 */
struct Preferences {
    /// Scope of archiving setting: forever or for the current stream only
    enum Scope { Global, Stream };
    static const Scope defaultScope = Global;

    /// Which part of the messaging stream should we store?
    enum Save {
        Body,       // Only <body> text
        False,      // Nothing
        Message,    // The whole <message> XML
        Stream      // The whole stream, byte-to-byte
    };

    /// How do we proceed Off-the-Record?
    enum Otr {
        Approve,    // the user MUST explicitly approve off-the-record communication.
        Concede,    // communications MAY be off the record if requested by another user.
        Forbid,     // communications MUST NOT be off the record.
        Oppose,     // communications SHOULD NOT be off the record even if requested.
        Prefer,     // communications SHOULD be off the record if possible.
        Require    // communications MUST be off the record.
    };

    /// Where and how hard does user prefer to store archive?
    enum Method {
        Auto,   // Server-side automatic archiving
        Local,  // Archiving to the local file/database
        Manual  // Manual server-side archiving
    };
    enum Use {
        Concede,    // Use if no other methods are available
        Forbid,     // Never use this method
        Prefer     // Use if available
    };
    static const uint defaultExpiration = (uint)-1;

    bool writePrefs(const QDomElement&);
    bool writePref(const QDomElement&);
    QString readPref(const QString &name);
    QDomElement uniformDomElement(const QString &name);

protected:
    bool setPref(const QString& name, const QVariant& value);

    bool handleAutoTag(const QDomElement&);
    bool handleDefaultTag(const QDomElement&);
    bool handleItemTag(const QDomElement&);
    bool handleSessionTag(const QDomElement&);
    bool handleMethodTag(const QDomElement&);

signals:
    void automaticArchivingEnable(bool,Preferences::Scope); // <auto save='true|false' scope='global|stream'/>
    void defaultPreferenceChanged(Preferences::Save,Preferences::Otr,uint); // <default save='#Save' otr='#Otr' expire='12345'/>
    void archivingMethodChanged(Preferences::Method,Preferences::Use); // <method type='auto|manual|local' use='concede|prefer|forbid'/>

private:
    static Scope toScope(const QString&);
    static Save toSave(const QString&);
    static Otr toOtr(const QString&);
    static Method toMethod(const QString&);
    static Use toUse(const QString&);
};
}
#endif
