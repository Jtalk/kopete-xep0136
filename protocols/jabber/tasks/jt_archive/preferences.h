#ifndef JT_ARCHIVE_PREFERENCES_H
#define JT_ARCHIVE_PREFERENCES_H

#include <QtCore/QString>
#include <QtCore/QSet>
#include <QtXml/QDomElement>


namespace JT_Archive_Helper {
/**
 * \brief The Preferences struct contains XEP-0136 preferences representation,
 * as well as string-to-enum and vice versa conversion routines.
 */
struct Preferences : public QObject{
    Q_OBJECT

public:
    /// Scope of archiving setting: forever or for the current stream only
    static const QSet<QString> legalScopes;
    static const QString defaultScope;

    /// Which part of the messaging stream should we store?
    static const QSet<QString> legalSaveModes;

    /// How do we proceed Off-the-Record?
    static const QSet<QString> legalOtrModes;

    /// Where and how hard does user prefer to store archive?
    static const QSet<QString> legalStorages;
    static const QSet<QString> legalStoragePriorities;
    static const uint defaultExpiration = (uint)-1;

    Preferences();

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
    void automaticArchivingEnable(bool,QString scope); // <auto save='true|false' scope='global|stream'/>
    void defaultPreferenceChanged(QString saveMode,QString otr,uint expire); // <default save='#Save' otr='#Otr' expire='12345'/>
    void archivingMethodChanged(QString method,QString use); // <method type='auto|manual|local' use='concede|prefer|forbid'/>

private:
    static bool verifyScope(const QString&);
    static bool verifySaveMode(const QString&);
    static bool verifyOtr(const QString&);
    static bool verifyStorageMethod(const QString&);
    static bool verifyStoragePriority(const QString&);
};
}
#endif
