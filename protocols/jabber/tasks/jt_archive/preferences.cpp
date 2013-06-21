#include "preferences.h"


JT_Archive_Helper::Preferences::Scope JT_Archive_Helper::Preferences::toScope(const QString &s)
{
    if (s == "global") {
        return Global;
    } else if (s == "stream") {
        return Stream;
    } else return defaultScope;
}

JT_Archive_Helper::Preferences::Save JT_Archive_Helper::Preferences::toSave(const QString &s)
{
    if (s == "body") {
        return Body;
    } else if (s == "false") {
        return False;
    } else if (s == "message") {
        return Message;
    } else if (s == "stream") {
        return Stream;
    } else {
        return -1;
    }
}

JT_Archive_Helper::Preferences::Otr JT_Archive_Helper::Preferences::toOtr(const QString &s)
{
    if (s == "approve") {
        return Approve;
    } else if (s == "concede") {
        return Concede;
    } else if (s == "forbid") {
        return Forbid;
    } else if (s == "oppose") {
        return Oppose;
    } else if (s == "prefer") {
        return Prefer;
    } else if (s == "require") {
        return Require;
    } else {
        return -1;
    }
}
}

JT_Archive_Helper::Preferences::Method JT_Archive_Helper::Preferences::toMethod(const QString &s)
{
    if (s == "auto") {
        return Auto;
    }else if (s == "local") {
        return Local;
    }else if (s == "manual") {
        return Manual;
    } else {
        return -1;
    }
}

JT_Archive_Helper::Preferences::Use JT_Archive_Helper::Preferences::toUse(const QString &s)
{
    if (s == "concede") {
        return Concede;
    } else if (s == "forbid") {
        return Forbid;
    } else if (s == "prefer") {
        return Prefer;
    } else {
        return -1;
    }
}

bool verifyAutoTag(const QDomElement &autoTag)
{
    return autoTag.childNodes().isEmpty()
            && autoTag.attributes().contains("save");
}

bool JT_Archive_Helper::Preferences::handleAutoTag(const QDomElement &autoTag)
{
    // <auto> Must be an empty tag and must contain 'save' attribute
    // according to the current standard draft
    if (verifyAutoTag(autoTag)) {
        bool isAutoArchivingEnabled = QVariant(autoTag.attribute("save")).toBool();

        // This will return either received scope or static defaultScope variable,
        // if no correct scope is received;
        Scope scope = toScope(autoTag.attribute("scope"));

        emit automaticArchivingEnable(isAutoArchivingEnabled, scope);
        return true;
    } else {
        return false;
    }
}

bool verifyDefaultTag(const QDomElement &defaultTag)
{
    return defaultTag.childNodes().isEmpty()
            && defaultTag.attributes().contains("save")
            && defaultTag.attributes().contains("otr");
}

bool JT_Archive_Helper::Preferences::handleDefaultTag(const QDomElement &defaultTag)
{
    // <default> Must be an empty tag and must contain 'save' and 'otr' attributes
    // according to the current standard draft
    if (verifyDefaultTag(defaultTag)) {
        Save saveMode = toSave(defaultTag.attribute("save"));
        Otr otrMode = toOtr(defaultTag.attribute("otr"));

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
#error NOIMPLEMENT
}

bool JT_Archive_Helper::Preferences::handleSessionTag(const QDomElement &)
{
#error NOIMPLEMENT
}

bool verifyMethodTag(const QDomElement &tag) {
    return elem.childNodes().isEmpty()
            && elem.attributes().contains("type")
            && elem.attributes().contains("use");
}

bool JT_Archive_Helper::Preferences::handleMethodTag(const QDomElement &methodTag)
{
    if (verifyMethodTag(methodTag)) {
        QString method = methodTag.attribute("type");
        QString use = methodTag.attribute("use");
        emit archivingMethodChanged(toMethod(method), toUse(use));
        return true;
    } else return false;
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
        qDebug() << "Unknown tag: " << elem.text() << endl;
        return false;
    }
}
