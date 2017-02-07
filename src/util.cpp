#include "util.h"

#include <QByteArray>
#include <QStringList>

static QString impl__to_human_readable_binary_data(char c) {
	if (c < ' ') {
		if (c == '\t' || c == '\n') {
			return {c};
		}
	} else if (c <= '~') {
		if (c != '%') {
			return {c};
		}
	}
	char buffer[8];
	snprintf(buffer, sizeof buffer, "%%%02X", static_cast<unsigned char>(c));
	return buffer;
}

QString Utility::to_human_readable_binary_data(const QByteArray &data) {
	QString retval;
	for (auto &c : data) {
		retval += impl__to_human_readable_binary_data(c);
	}
	return retval;
}

QString Utility::to_human_readable_binary_data(const QString &data) {
	QString retval;
	for (auto &c : data) {
		retval += impl__to_human_readable_binary_data(c.toLatin1());
	}
	return retval;
}

QString Utility::to_C_hex_encoding(const QByteArray &data) {
	QStringList l;
	for (char c : data) {
		auto hex = QString::number(static_cast<unsigned char>(c), 16);
		if (hex.size() == 1) {
			l << "0x0" + hex;
		} else {
			l << "0x" + hex;
		}
	}
	return l.join(",");
}
