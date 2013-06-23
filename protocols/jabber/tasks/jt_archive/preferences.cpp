#include "preferences.h"
#include <QtCore/QVariant>
#include <QtCore/QDebug>

const QSet<QString> JT_Archive_Helper::Preferences::legalScopes = QSet<QString>() << "global" << "stream";
const QString  JT_Archive_Helper::Preferences::defaultScope = "global";
const QSet<QString>  JT_Archive_Helper::Preferences::legalSaveModes = QSet<QString>()
        << "body"
        << "false"
        << "message"
        << "stream";
const QSet<QString>  JT_Archive_Helper::Preferences::legalOtrModes = QSet<QString>()
        << "concede"
        << "prefer"
        << "forbid"
        << "approve"
        << "oppose"
        << "require";
const QSet<QString>  JT_Archive_Helper::Preferences::legalStorages = QSet<QString>()
        << "auto"
        << "local"
        << "manual";
const QSet<QString>  JT_Archive_Helper::Preferences::legalStoragePriorities = QSet<QString>()
        << "forbid"
        << "concede"
        << "prefer";

static inline bool verifyAutoTag(const QDomElement &autoTag)
{
    return autoTag.childNodes().isEmpty()
            && autoTag.attributes().contains("save");
}

static inline bool hasScope(const QDomElement &autoTag)
{
    return autoTag.attributes().contains("scope");
}

bool JT_Archive_Helper::Preferences::handleAutoTag(const QDomElement &autoTag)
{
    // This will return either received scope or static defaultScope variable,
    // if no correct scope is received;
    QString scope = hasScope(autoTag) ? autoTag.attribute("scope") : defaultScope;
    // <auto> Must be an empty tag and must contain 'save' attribute
    // according to the current standard draft.
    // If there was no scope, 'scope' variable would be defaultScope, which is
    // valid. If there was a 'scope' tag in an incoming <iq/>, we should check
    // it's value.
    if (verifyAutoTag(autoTag) && verifyScope(scope)) {
        bool isAutoArchivingEnabled = QVariant(autoTag.attribute("save")).toBool();
        emit automaticArchivingEnable(isAutoArchivingEnabled, scope);
        return true;
    } else {
        return false;
    }
}

static inline bool verifyDefaultTag(const QDomElement &defaultTag)
{
    return defaultTag.childNodes().isEmpty()
            && defaultTag.attributes().contains("save")
            && defaultTag.attributes().contains("otr");
}

bool JT_Archive_Helper::Preferences::handleDefaultTag(const QDomElement &defaultTag)
{
    QString saveMode = defaultTag.attribute("save");
    QString otrMode = defaultTag.attribute("otr");
    // <default> Must be an empty tag and must contain 'save' and 'otr' attributes
    // according to the current standard draft.
    if (verifyDefaultTag(defaultTag) && verifySaveMode(saveMode) && verifyOtr(otrMode)) {

        uint expire = defaultTag.attributes().contains("expire")
                ? QVariant(defaultTag.attribute("scope")).toUInt()
                : defaultExpiration;
        emit defaultPreferenceChanged(saveMode, otrMode, expire);
        return true;
    } else {
        return false;
    }
}

bool JT_Archive_Helper::Preferences::handleItemTag(const QDomElement &)
{
    // TODO: Implement per-user storing settings
    return true;
}

bool JT_Archive_Helper::Preferences::handleSessionTag(const QDomElement &)
{
    // TODO: Implement per-session storing settings
    return true;
}

static inline bool verifyMethodTag(const QDomElement &tag) {
    return tag.childNodes().isEmpty()
            && tag.attributes().contains("type")
            && tag.attributes().contains("use");
}

bool JT_Archive_Helper::Preferences::handleMethodTag(const QDomElement &methodTag)
{
    QString method = methodTag.attribute("type");
    QString use = methodTag.attribute("use");
    if (verifyMethodTag(methodTag) && verifyStorageMethod(method) && verifyStoragePriority(use)) {
        emit archivingMethodChanged(method, use);
        return true;
    } else return false;
}

JT_Archive_Helper::Preferences::Preferences()
{
}

#define DOM_FOREACH(var, domElement) for(QDomNode var = domElement.firstChild(); !var.isNull(); var = var.nextSibling())

bool JT_Archive_Helper::Preferences::writePrefs(const QDomElement &tags)
{
    bool isSuccessful = true;
    DOM_FOREACH(tag, tags) {
        isSuccessful = isSuccessful && writePref(tag.toElement());
    }
    return isSuccessful;
}

bool JT_Archive_Helper::Preferences::writePref(const QDomElement &elem)
{
    QString tagName = elem.tagName();
    if (tagName == "auto") {
        return handleAutoTag(elem);
    } else if (tagName == "default") {
        return handleDefaultTag(elem);
    } else if (tagName == "item") {
        return handleItemTag(elem);
    } else if (tagName == "session") {
        return handleSessionTag(elem);
    } else if (tagName == "method") {
        return handleMethodTag(elem);
    } else {
        // Unknown tag?
        qDebug() << "Unknown tag: " << elem.text();
        return false;
    }
}

bool JT_Archive_Helper::Preferences::verifyScope(const QString &s)
{
    return legalScopes.contains(s.toLower());
}

bool JT_Archive_Helper::Preferences::verifySaveMode(const QString &s)
{
    return legalSaveModes.contains(s.toLower());
}

bool JT_Archive_Helper::Preferences::verifyOtr(const QString &s)
{
    return legalOtrModes.contains(s.toLower());
}

bool JT_Archive_Helper::Preferences::verifyStorageMethod(const QString &s)
{
    return legalStorages.contains(s.toLower());
}

bool JT_Archive_Helper::Preferences::verifyStoragePriority(const QString &s)
{
    return legalStoragePriorities.contains(s.toLower());
}
