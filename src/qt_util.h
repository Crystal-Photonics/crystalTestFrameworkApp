#ifndef QT_UTIL_H
#define QT_UTIL_H

#include <QVariant>

namespace Utility {
	inline QVariant make_qvariant(void *p) {
		return QVariant::fromValue(p);
	}
	template <class T>
	inline T *from_qvariant(QVariant &qv) {
		return static_cast<T *>(qv.value<void *>());
	}
	template <class T>
	inline T *from_qvariant(QVariant &&qv) {
		return static_cast<T *>(qv.value<void *>());
	}
}

#endif // QT_UTIL_H
