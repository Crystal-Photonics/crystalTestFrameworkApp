#include "data_engine.h"
#include "util.h"

#include <QByteArray>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <algorithm>
#include <iostream>
//#include <qtrpt.h>
#include <type_traits>

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
	const auto object = document.object();
	const auto array_object = object[""];
	if (!array_object.isArray()) {
		throw std::runtime_error("invalid json file");
	}
	const auto array = array_object.toArray();
	for (const auto &array_element : array) {
		const auto object = array_element.toObject();
		add_entry(Data_engine_entry::from_json(object));
	}
}

bool Data_engine::is_complete() const {
	return std::all_of(std::begin(entries), std::end(entries), [](const auto &entry) { return entry.second->is_complete(); });
}

bool Data_engine::all_values_in_range() const {
	return std::all_of(std::begin(entries), std::end(entries), [](const auto &entry) { return entry.second->is_in_range(); });
}

bool Data_engine::value_in_range(const FormID &id) const {
	auto entry = get_entry(id);
	if (entry == nullptr) {
		return false;
	}
	return entry->is_in_range();
}

void Data_engine::set_actual_number(const FormID &id, double number) {
	get_entry(id)->as<Numeric_entry>().actual_value = number;
}

void Data_engine::set_actual_text(const FormID &id, std::string text) {
	get_entry(id)->as<Text_entry>().actual_value = std::move(text);
}

double Data_engine::get_desired_value(const FormID &id) const {
	return get_entry(id)->as<Numeric_entry>().target_value;
}

double Data_engine::get_desired_absolute_tolerance(const FormID &id) const {
	return get_entry(id)->as<Numeric_entry>().deviation;
}

double Data_engine::get_desired_relative_tolerance(const FormID &id) const {
	return get_entry(id)->as<Numeric_entry>().deviation / get_entry(id)->as<Numeric_entry>().target_value;
}

double Data_engine::get_desired_minimum(const FormID &id) const {
	return get_entry(id)->as<Numeric_entry>().target_value - get_entry(id)->as<Numeric_entry>().deviation;
}

double Data_engine::get_desired_maximum(const FormID &id) const {
	return get_entry(id)->as<Numeric_entry>().target_value + get_entry(id)->as<Numeric_entry>().deviation;
}

const std::string &Data_engine::get_unit(const FormID &id) const {
	return get_entry(id)->as<Numeric_entry>().unit;
}
#if 0
std::unique_ptr<QWidget> Data_engine::get_preview() const {

	QtRPT report;
	report.printExec();

    return nullptr;
}
#endif

void Data_engine::add_entry(std::pair<FormID, std::unique_ptr<Data_engine_entry>> &&entry) {
	auto pos = std::lower_bound(std::begin(entries), std::end(entries), entry, entry_compare);
	entries.insert(pos, std::move(entry));
}

Data_engine_entry *Data_engine::get_entry(const FormID &id) {
	return const_cast<Data_engine_entry *>(Utility::as_const(*this).get_entry(id));
}

const Data_engine_entry *Data_engine::get_entry(const FormID &id) const {
	auto pos = std::lower_bound(std::begin(entries), std::end(entries), id, entry_compare);
	if (pos == std::end(entries)) {
		return nullptr;
	}
	return pos->second.get();
}

bool Data_engine::entry_compare(FormIdWrapper lhs, FormIdWrapper rhs) {
	return lhs.value < rhs.value;
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
		std::string unit{};

		for (const auto &key : keys) {
			if (key == "name") {
				form = object.value(key).toString().toStdString();
			} else if (key == "value") {
				target_value = object.value(key).toDouble(0.);
			} else if (key == "deviation") {
				deviation = object.value(key).toDouble(0.);
			} else if (key == "unit") {
				unit = object.value(key).toString().toStdString();
			} else {
				throw std::runtime_error("Invalid key \"" + key.toStdString() + "\" in numeric JSON object");
			}
		}
		return {std::move(form), std::make_unique<Numeric_entry>(target_value, deviation, std::move(unit))};
	} else if (value.isString()) {
		std::string target_value{};
		for (const auto &key : keys) {
			if (key == "name") {
				form = object.value(key).toString().toStdString();
			} else if (key == "value") {
				target_value = object.value(key).toString().toStdString();
			} else {
				throw std::runtime_error("Invalid key \"" + key.toStdString() + "\" in textual JSON object");
			}
		}
		return {std::move(form), std::make_unique<Text_entry>(target_value)};
	}
	throw std::runtime_error("invalid JSON object");
}

Numeric_entry::Numeric_entry(double target_value, double deviation, std::string unit)
	: target_value(target_value)
	, deviation(deviation)
	, unit(std::move(unit)) {}

bool Numeric_entry::is_complete() const {
	return bool(actual_value);
}

bool Numeric_entry::is_in_range() const {
	return is_complete() && std::abs(actual_value.value() - target_value) <= deviation;
}

Text_entry::Text_entry(std::string target_value)
	: target_value(std::move(target_value)) {}

bool Text_entry::is_complete() const {
	return bool(actual_value);
}

bool Text_entry::is_in_range() const {
	return is_complete() && actual_value.value() == target_value;
}
