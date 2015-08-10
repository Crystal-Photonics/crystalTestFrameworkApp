#ifndef UTILITY_H
#define UTILITY_H

#include <sstream>
#include <QString>

namespace Utility{
	template<class T>
	bool convert(const std::wstring &ws, T &t){
		return !(std::wistringstream(ws) >> t).fail();
	}
	template<class T>
	bool convert(const std::string &s, T &t){
		return !(std::istringstream(s) >> t).fail();
	}
	template<class T>
	bool convert(const QString &qs, T &t){
		return convert(qs.toStdWString(), t);
	}
}

#endif // UTILITY_H
