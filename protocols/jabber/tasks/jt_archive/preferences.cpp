#include "jt_archive.h"
#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtCore/QStringBuilder>

/// We need capitalized properties names for QMetaEnum
inline QString capitalize(const QString &str)
{
    QString loweredStr = str.toLower();
    loweredStr[0] = loweredStr[0].toUpper();
    return loweredStr;
}

static inline bool verifyAutoTag(const QDomElement &autoTag)
{
    return autoTag.childNodes().isEmpty()
            && autoTag.attributes().contains("save");
}

static inline bool hasScope(const QDomElement &autoTag)
{
    return autoTag.attributes().contains("scope");
}

#define STR(x) # x
#define STRCAT(x,y) QString(QString(x) + "_" + QString(y))
#define EXTRACT_TAG(type, tag, name) (type)s_ ## type ## Enum.keyToValue(STRCAT(STR(type) \
    , tag.attribute(name)).toAscii())

bool JT_Archive::handleAutoTag(const QDomElement &autoTag)
{
    // This will return either received scope or static defaultScope variable,
    // if no correct scope is received;
    AutoScope scope = EXTRACT_TAG(AutoScope, autoTag, "scope");
    if (scope == -1) {
        scope = defaultScope;
    }
    // <auto> Must be an empty tag and must contain 'save' attribute
    // according to the current standard draft.
    // If there was no scope, 'scope' variable would be defaultScope, which is
    // valid.
    qDebug() << "autotagver" << (int)verifyAutoTag(autoTag);
    if (verifyAutoTag(autoTag)) {
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

bool JT_Archive::handleDefaultTag(const QDomElement &defaultTag)
{
    DefaultSave saveMode = EXTRACT_TAG(DefaultSave, defaultTag, "save");
    DefaultOtr otrMode = EXTRACT_TAG(DefaultOtr, defaultTag, "otr");
    // <default> Must be an empty tag and must contain 'save' and 'otr' attributes
    // according to the current standard draft.
    if (verifyDefaultTag(defaultTag) && saveMode != -1 && otrMode != -1) {
        uint expire = defaultTag.attributes().contains("expire")
                ? QVariant(defaultTag.attribute("scope")).toUInt()
                : defaultExpiration;
        emit defaultPreferenceChanged(saveMode, otrMode, expire);
        return true;
    } else {
        return false;
    }
}

bool JT_Archive::handleItemTag(const QDomElement &)
{
    // TODO: Implement per-user storing settings
    return true;
}

bool JT_Archive::handleSessionTag(const QDomElement &)
{
    // TODO: Implement per-session storing settings
    return true;
}

static inline bool verifyMethodTag(const QDomElement &tag) {
    return tag.childNodes().isEmpty()
            && tag.attributes().contains("type")
            && tag.attributes().contains("use");
}

bool JT_Archive::handleMethodTag(const QDomElement &methodTag)
{
    MethodType method = EXTRACT_TAG(MethodType, methodTag, "type");
    MethodUse use = EXTRACT_TAG(MethodUse, methodTag, "use");
    if (verifyMethodTag(methodTag) && method != -1 && use != -1) {
        emit archivingMethodChanged(method, use);
        return true;
    } else return false;
}

#define DOM_FOREACH(var, domElement) for(QDomNode var = domElement.firstChild(); !var.isNull(); var = var.nextSibling())

bool JT_Archive::writePrefs(const QDomElement &tags)
{
    bool isSuccessful = true;
    DOM_FOREACH(tag, tags) {
        bool isCurrentSuccessful = writePref(tag.toElement());
        isSuccessful = isSuccessful && isCurrentSuccessful;
    }
    return isSuccessful;
}

bool JT_Archive::writePref(const QDomElement &elem)
{
    TagNames tagName = (TagNames)s_TagNamesEnum.keyToValue(capitalize(elem.tagName()).toAscii());
    switch (tagName) {
    case Auto: return handleAutoTag(elem);
    case Default: return handleDefaultTag(elem);
    case Item: return handleItemTag(elem);
    case Session: return handleSessionTag(elem);
    case Method: return handleMethodTag(elem);
    default: return false;
    }
}

#define VALUE(type, value) QString( s_ ## type ## Enum.valueToKey(value) ).remove(QLatin1String(# type) + "_")
#define INSERT_TAG(type, tag, name, value) tag.setAttribute(name, VALUE(type, value))

void JT_Archive::updateDefault(const JT_Archive::DefaultSave save,
                               const JT_Archive::DefaultOtr otr,
                               const uint expiration)
{
    Q_UNUSED(expiration)
    QDomElement tag = doc()->createElement("default");
    INSERT_TAG(DefaultSave, tag, "save", save);
    INSERT_TAG(DefaultOtr, tag, "otr", otr);
    QDomElement update = uniformUpdate(tag);
    send(update);
}

void JT_Archive::updateAuto(bool isEnabled, const JT_Archive::AutoScope scope)
{
    QDomElement tag = doc()->createElement("auto");
    tag.setAttribute("save", QVariant(isEnabled).toString());
    if (scope != -1) {
        INSERT_TAG(AutoScope, tag, "scope", scope);
    }
    QDomElement update = uniformUpdate(tag);
    send(update);
}

void JT_Archive::updateStorage(const JT_Archive::MethodType method, const JT_Archive::MethodUse use)
{
    QDomElement tag = doc()->createElement("method");
    INSERT_TAG(MethodType, tag, "type", method);
    INSERT_TAG(MethodUse, tag, "use", use);
    QDomElement update = uniformUpdate(tag);
    send(update);
}
