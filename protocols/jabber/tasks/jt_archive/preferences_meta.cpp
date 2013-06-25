#include "jt_archive.h"

const QMetaEnum JT_Archive::s_TagNamesEnum = JT_Archive::staticMetaObject.enumerator( JT_Archive::staticMetaObject.indexOfEnumerator( "TagNames" ) );
const QMetaEnum JT_Archive::s_AutoScopeEnum = JT_Archive::staticMetaObject.enumerator( JT_Archive::staticMetaObject.indexOfEnumerator( "AutoScope" ) );
const QMetaEnum JT_Archive::s_DefaultSaveEnum = JT_Archive::staticMetaObject.enumerator( JT_Archive::staticMetaObject.indexOfEnumerator( "DefaultSave" ) );
const QMetaEnum JT_Archive::s_DefaultOtrEnum = JT_Archive::staticMetaObject.enumerator( JT_Archive::staticMetaObject.indexOfEnumerator( "DefaultOtr" ) );
const QMetaEnum JT_Archive::s_MethodTypeEnum = JT_Archive::staticMetaObject.enumerator( JT_Archive::staticMetaObject.indexOfEnumerator( "MethodType" ) );
const QMetaEnum JT_Archive::s_MethodUseEnum = JT_Archive::staticMetaObject.enumerator( JT_Archive::staticMetaObject.indexOfEnumerator( "MethodUse" ) );

const JT_Archive::AutoScope JT_Archive::defaultScope = JT_Archive::AutoScope_global;
