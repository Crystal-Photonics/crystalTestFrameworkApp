#include "data_engine.h"
#include "util.h"

#include <QApplication>
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <algorithm>
#include <iostream>
#include <qtrpt.h>
#include <type_traits>

constexpr auto unavailable_value = "/";

Data_engine::Data_engine(std::istream &source) {
	QByteArray data;
	constexpr auto eof = std::remove_reference<decltype(source)>::type::traits_type::eof();
	while (source) {
		auto character = source.get();
		if (character == eof) {
			break;
		}
		data.push_back(character);
	}
	const auto document = QJsonDocument::fromJson(std::move(data));
	if (!document.isArray()) {
		throw std::runtime_error("invalid json file");
	}
	const auto array = document.array();
	for (const auto &array_element : array) {
		const auto object = array_element.toObject();
		add_entry(Data_engine_entry::from_json(object));
	}
}

bool Data_engine::is_complete() const {
	return std::all_of(std::begin(id_entries), std::end(id_entries), [](const auto &entry) { return entry.second->is_complete(); }) &&
		   std::all_of(std::begin(data_entries), std::end(data_entries), [](const auto &entry) { return entry->is_complete(); });
}

bool Data_engine::all_values_in_range() const {
	return std::all_of(std::begin(id_entries), std::end(id_entries), [](const auto &entry) { return entry.second->is_in_range(); }) &&
		   std::all_of(std::begin(data_entries), std::end(data_entries), [](const auto &entry) { return entry->is_in_range(); });
}

bool Data_engine::value_in_range(const FormID &id) const {
	auto entry = get_entry(id);
	if (entry == nullptr) {
		return false;
	}
	return entry->is_in_range();
}

void Data_engine::set_actual_number(const FormID &id, double number) {
	get_entry(id)->as<Numeric_entry>()->actual_value = number;
}

void Data_engine::set_actual_text(const FormID &id, QString text) {
	get_entry(id)->as<Text_entry>()->actual_value = std::move(text);
}

double Data_engine::get_desired_value(const FormID &id) const {
	return get_entry(id)->as<Numeric_entry>()->target_value;
}

double Data_engine::get_desired_absolute_tolerance(const FormID &id) const {
	return get_entry(id)->as<Numeric_entry>()->deviation;
}

double Data_engine::get_desired_relative_tolerance(const FormID &id) const {
	return get_entry(id)->as<Numeric_entry>()->deviation / get_entry(id)->as<Numeric_entry>()->target_value;
}

double Data_engine::get_desired_minimum(const FormID &id) const {
	return get_entry(id)->as<Numeric_entry>()->target_value - get_entry(id)->as<Numeric_entry>()->deviation;
}

double Data_engine::get_desired_maximum(const FormID &id) const {
	return get_entry(id)->as<Numeric_entry>()->target_value + get_entry(id)->as<Numeric_entry>()->deviation;
}

const QString &Data_engine::get_unit(const FormID &id) const {
	return get_entry(id)->as<Numeric_entry>()->unit;
}

Data_engine::Statistics Data_engine::get_statistics() const {
	int number_of_id_fields = id_entries.size();
	int number_of_data_fields = data_entries.size();
	int number_of_filled_fields = std::accumulate(std::begin(id_entries), std::end(id_entries), 0,
												  [](int value, const auto &entry) { return value + (entry.second->is_complete() ? 1 : 0); }) +
								  std::accumulate(std::begin(data_entries), std::end(data_entries), 0,
												  [](int value, const auto &entry) { return value + (entry->is_complete() ? 1 : 0); });
	int number_of_inrange_fields = std::accumulate(std::begin(id_entries), std::end(id_entries), 0,
												   [](int value, const auto &entry) { return value + (entry.second->is_in_range() ? 1 : 0); }) +
								   std::accumulate(std::begin(data_entries), std::end(data_entries), 0,
												   [](int value, const auto &entry) { return value + (entry->is_in_range() ? 1 : 0); });

	return {number_of_id_fields, number_of_data_fields, number_of_filled_fields, number_of_inrange_fields};
}

std::unique_ptr<QWidget> Data_engine::get_preview() const {
	QtRPT report;
	fill_report(report, "test.xml");
	report.printExec();
	return nullptr;
}

void Data_engine::generate_pdf(const std::string &path) const {
	QtRPT report;
	fill_report(report, "test.xml");
	report.printPDF(QString::fromStdString(path));
}

void Data_engine::add_entry(std::pair<FormID, std::unique_ptr<Data_engine_entry>> &&entry) {
	if (entry.first.isEmpty()) {
		auto pos = std::lower_bound(std::begin(data_entries), std::end(data_entries), entry, entry_compare);
		data_entries.insert(pos, std::move(entry.second));
	} else {
		auto pos = std::lower_bound(std::begin(id_entries), std::end(id_entries), entry, entry_compare);
		id_entries.insert(pos, std::move(entry));
	}
}

Data_engine_entry *Data_engine::get_entry(const FormID &id) {
	return const_cast<Data_engine_entry *>(Utility::as_const(*this).get_entry(id));
}

const Data_engine_entry *Data_engine::get_entry(const FormID &id) const {
	{
		auto pos = std::lower_bound(std::begin(id_entries), std::end(id_entries), id, entry_compare);
		if (pos != std::end(id_entries) && pos->first == id) {
			return pos->second.get();
		}
	}
	{
		auto pos = std::lower_bound(std::begin(data_entries), std::end(data_entries), id, entry_compare);
		if (pos != std::end(data_entries) && pos->get()->get_description() == id) {
			return pos->get();
		}
	}
	return nullptr;
}

bool Data_engine::entry_compare(FormIdWrapper lhs, FormIdWrapper rhs) {
	return lhs.value < rhs.value;
}

void Data_engine::setValue(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage) const {
	const auto &entry = data_entries[recNo];
	if (paramName == "DataDescription") {
		paramValue = entry->get_description();
	} else if (paramName == "DataMinimum") {
		paramValue = entry->get_minimum();
	} else if (paramName == "DataMaximum") {
		paramValue = entry->get_maximum();
	} else if (paramName == "DataValue") {
		paramValue = entry->get_value();
	} else if (paramName == "DataSuccess") {
		paramValue = entry->is_in_range() ? "Ok" : "Fail";
	} else if (paramName == "DateTime") {
		paramValue = QDateTime::currentDateTime().toString(Qt::SystemLocaleLongDate);
	} else if (paramName == "ReportSummary") {
		paramValue = get_statistics().to_qstring();
	} else if (paramName == "TestSuccess") {
		paramValue = all_values_in_range() ? 1 : 0;
	} else {
		auto entry = get_entry(paramName);
		if (entry != nullptr) {
			paramValue = entry->get_value();
		}
	}
}

void Data_engine::fill_report(QtRPT &report, const QString &form) const {
	if (report.loadReport(form) == false) {
		throw std::runtime_error("Failed opening form file " + form.toStdString());
	}
	report.recordCount << data_entries.size();
    #warning is there an alternative to qOverload since qOverload is very new (since qt 5.7)
	QObject::connect(
		&report, qOverload<const int, const QString, QVariant &, const int>(&QtRPT::setValue),
		[this](const int recNo, const QString paramName, QVariant &paramValue, const int reportPage) { setValue(recNo, paramName, paramValue, reportPage); });
}

std::pair<FormID, std::unique_ptr<Data_engine_entry>> Data_engine_entry::from_json(const QJsonObject &object) {
	FormID form;
	const auto keys = object.keys();
	if (!keys.contains("value")) {
		throw std::runtime_error("JSON object must contain a key \"value\"");
	}
	const auto &value = object.value("value");
	if (value.isDouble()) {
		double target_value{};
		double deviation{};
		QString unit{};
		QString description{};

		for (const auto &key : keys) {
			if (key == "id") {
				form = object.value(key).toString();
			} else if (key == "name") {
				description = object.value(key).toString();
			} else if (key == "value") {
				target_value = object.value(key).toDouble(0.);
			} else if (key == "deviation") {
				deviation = object.value(key).toDouble(0.);
			} else if (key == "unit") {
				unit = object.value(key).toString();
			} else {
				throw std::runtime_error("Invalid key \"" + key.toStdString() + "\" in numeric JSON object");
			}
		}
		return {std::move(form), std::make_unique<Numeric_entry>(target_value, deviation, std::move(unit), std::move(description))};
	} else if (value.isString()) {
		QString target_value{};
		for (const auto &key : keys) {
			if (key == "name") {
				form = object.value(key).toString();
			} else if (key == "value") {
				target_value = object.value(key).toString();
			} else {
				throw std::runtime_error("Invalid key \"" + key.toStdString() + "\" in textual JSON object");
			}
		}
		return {std::move(form), std::make_unique<Text_entry>(target_value)};
	}
	throw std::runtime_error("invalid JSON object");
}

Numeric_entry::Numeric_entry(double target_value, double deviation, QString unit, QString description)
	: target_value(target_value)
	, deviation(deviation)
	, unit(std::move(unit))
	, description(std::move(description)) {}

bool Numeric_entry::is_complete() const {
	return bool(actual_value);
}

bool Numeric_entry::is_in_range() const {
	return is_complete() && std::abs(actual_value.value() - target_value) <= deviation;
}

QString Numeric_entry::get_value() const {
	return is_complete() ? QString::number(actual_value.value()) : unavailable_value;
}

QString Numeric_entry::get_description() const {
	return description;
}

QString Numeric_entry::get_minimum() const {
	return QString::number(get_min_value());
}

QString Numeric_entry::get_maximum() const {
	return QString::number(get_max_value());
}

double Numeric_entry::get_min_value() const {
	return target_value - deviation;
}

double Numeric_entry::get_max_value() const {
	return target_value + deviation;
}

Text_entry::Text_entry(QString target_value)
	: target_value(std::move(target_value)) {}

bool Text_entry::is_complete() const {
	return bool(actual_value);
}

bool Text_entry::is_in_range() const {
	return is_complete() && actual_value.value() == target_value;
}

QString Text_entry::get_value() const {
	return is_complete() ? actual_value.value() : unavailable_value;
}

QString Text_entry::get_description() const {
	return description;
}

QString Text_entry::get_minimum() const {
	return "";
}

QString Text_entry::get_maximum() const {
	return "";
}

QString Data_engine::Statistics::to_qstring() const {
	const int total = number_of_id_fields + number_of_data_fields;
	return QString(R"(Number of fields: %1
Fields filled: %2/%3
Fields succeeded: %4/%5)")
		.arg(total) //
		.arg(number_of_filled_fields)
		.arg(total)
		.arg(number_of_inrange_fields)
		.arg(total);
}
