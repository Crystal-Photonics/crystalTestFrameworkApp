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
#include <cassert>
#include <iostream>
#include <limits>
#include <qtrpt.h>
#include <type_traits>

template <class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type almost_equal(T x, T y, int ulp) {
    // the machine epsilon has to be scaled to the magnitude of the values used
    // and multiplied by the desired precision in ULPs (units in the last place)
    return std::abs(x - y) < std::numeric_limits<T>::epsilon() * std::abs(x + y) * ulp
           // unless the result is subnormal
           || std::abs(x - y) < std::numeric_limits<T>::min();
}

bool is_comment_key(const QString &keyname) {
    return keyname.startsWith("_");
}

constexpr auto unavailable_value = "/";

Data_engine::Data_engine(std::istream &source) {
    set_source(source);
}

void Data_engine::set_source(std::istream &source) {
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
    if (!document.isObject()) {
        throw std::runtime_error("invalid json file");
    }

    sections.from_json(document.object());
}

const DataEngineDataEntry *DataEngineSections::get_entry(const FormID &id, const QMap<QString, QVariant> &tags) const {
    auto names = id.split("/");
    if (names.count() != 2) {
        throw DataEngineError(DataEngineErrorNumber::faulty_field_id, QString("field id needs to be in format \"section-name/field-name\" but is %1").arg(id));
    }
    bool section_found = false;
    QString section_name = names[0];
    QString field_name = names[1];
    for (auto &section : sections) {
        if (section.get_section_name() == section_name) {
            const VariantData *variant = section.get_variant(tags);
            assert(variant);

            section_found = true;
            const DataEngineDataEntry *result = variant->get_value(field_name);
            if (result == nullptr) {
                throw DataEngineError(DataEngineErrorNumber::no_field_id_found, QString("Could not find field with name = \"%1\"").arg(field_name));
            }
            return result;
        }
    }
    if (!section_found) {
        throw DataEngineError(DataEngineErrorNumber::no_section_id_found, QString("Could not find section with name = \"%1\"").arg(section_name));
    }
    return nullptr;
}

DataEngineDataEntry *DataEngineSections::get_entry(const FormID &id, const QMap<QString, QVariant> &tags) {
    return const_cast<DataEngineDataEntry *>(Utility::as_const(*this).get_entry(id, tags));
}

bool DataEngineSections::is_complete(const QMap<QString, QVariant> &tags) const {
    bool result = true;
    for (const auto &section : sections) {
        if (!section.is_complete(tags)) {
            result = false;
        }
    }
    return result;
}

bool DataEngineSections::all_values_in_range(const QMap<QString, QVariant> &tags) const {
    bool result = true;
    for (const auto &section : sections) {
        if (!section.all_values_in_range(tags)) {
            result = false;
        }
    }
    return result;
}

void DataEngineSections::from_json(const QJsonObject &object) {
    for (const auto &key : object.keys()) {
        if (is_comment_key(key)) {
            continue;
        }
        auto section_object = object[key];
        if (!(section_object.isObject() || section_object.isArray())) {
            throw std::runtime_error(QString("invalid json object %1").arg(key).toStdString());
        }
        DataEngineSection section;

        section.from_json(section_object, key);
        if (section_exists(section.get_section_name())) {
            throw DataEngineError(DataEngineErrorNumber::duplicate_section, QString("duplicate section %1").arg(section.get_section_name()));
        }
        sections.push_back(std::move(section));
    }
}

bool DataEngineSections::section_exists(QString section_name) {
    bool result = false;
    for (const auto &section : sections) {
        if (section.get_section_name() == section_name) {
            result = true;
            break;
        }
    }
    return result;
}

const VariantData *DataEngineSection::get_variant(const QMap<QString, QVariant> &tags) const {
    bool matched = false;
    const VariantData *result = nullptr;
    for (auto &variant : variants) {
        if (variant.is_dependency_matching(tags)) {
            if (matched) {
                throw DataEngineError(DataEngineErrorNumber::non_unique_desired_field_found,
                                      QString("More than one dependency fullfilling variant found in section: \"%1\"").arg(section_name));
            }
            result = &variant;
            matched = true;
        }
    }
    if (!matched) {
        throw DataEngineError(DataEngineErrorNumber::non_unique_desired_field_found,
                              QString("No dependency fullfilling variant found in section: \"%1\"").arg(section_name));
    }
    return result;
}

bool DataEngineSection::is_complete(const QMap<QString, QVariant> &tags) const {
    bool result = true;
    const VariantData *variant_to_test = get_variant(tags);
    for (const auto &entry : variant_to_test->data_entries) {
        if (!entry.get()->is_complete()) {
            result = false;
        }
    }
    return result;
}

bool DataEngineSection::all_values_in_range(const QMap<QString, QVariant> &tags) const {
    bool result = true;
    const VariantData *variant_to_test = get_variant(tags);
    for (const auto &entry : variant_to_test->data_entries) {
        if (!entry.get()->is_in_range()) {
            result = false;
        }
    }
    return result;
}

void DataEngineSection::from_json(const QJsonValue &object, const QString &key_name) {
    section_name = key_name;
    if (object.isArray()) {
        for (auto variant : object.toArray()) {
            append_variant_from_json(variant.toObject());
        }
    } else if (object.isObject()) {
        append_variant_from_json(object.toObject());
    } else {
        throw std::runtime_error(QString("invalid variant in json file").toStdString());
    }
}

QString DataEngineSection::get_section_name() const {
    return section_name;
}

void DataEngineSection::append_variant_from_json(const QJsonObject &object) {
    VariantData variant;
    variant.from_json(object);
    variants.push_back(std::move(variant));
}

bool VariantData::is_dependency_matching(const QMap<QString, QVariant> &tags) const {
    bool dependency_matches = true;

    const auto &keys_to_test = dependency_tags.tags.uniqueKeys();
    for (const auto &tag_key : keys_to_test) {
        if (tags.contains(tag_key)) {
            bool at_least_value_matches = false;
            const auto &value_list = dependency_tags.tags.values(tag_key);
            for (const auto &test_matching : value_list) {
                if (test_matching.is_matching(tags[tag_key])) {
                    at_least_value_matches = true;
                }
            }
            if (!at_least_value_matches) {
                dependency_matches = false;
                break;
            }
        } else {
            dependency_matches = false;
        }
    }

    return dependency_matches;
}

void VariantData::from_json(const QJsonObject &object) {
    auto depend_tags = object["apply_if"];
    QJsonValue data = object["data"];

    if (data.isUndefined() || data.isNull()) {
        throw DataEngineError(DataEngineErrorNumber::no_data_section_found, QString("no data found ininvalid variant in json file"));
    }

    if (data.isArray()) {
        if (data.toArray().count() == 0) {
            throw DataEngineError(DataEngineErrorNumber::no_data_section_found, QString("no data found in variant in json file"));
        }
        for (const auto &data_entry : data.toArray()) {
            std::unique_ptr<DataEngineDataEntry> entry = DataEngineDataEntry::from_json(data_entry.toObject());
            if (value_exists(entry.get()->field_name)) {
                throw DataEngineError(DataEngineErrorNumber::duplicate_field,
                                      QString("Data field with the name %1 is already existing").arg(entry.get()->field_name));
            }
            data_entries.push_back(std::move(entry));
        }
    } else if (data.isObject()) {
        std::unique_ptr<DataEngineDataEntry> entry = DataEngineDataEntry::from_json(data.toObject());
        if (value_exists(entry.get()->field_name)) {
            throw DataEngineError(DataEngineErrorNumber::duplicate_field,
                                  QString("Data field with the name %1 is already existing").arg(entry.get()->field_name));
        }
        data_entries.push_back(std::move(entry));
    }
    dependency_tags.from_json(depend_tags);
}

bool VariantData::value_exists(QString field_name) {
    return get_value(field_name) != nullptr;
}

DataEngineDataEntry *VariantData::get_value(QString field_name) const {
    for (const auto &entry : data_entries) {
        if (entry.get()->field_name == field_name) {
            return entry.get();
        }
    }
    return nullptr;
}

void DependencyTags::from_json(const QJsonValue &object) {
    tags.clear();
    if (object.isObject()) {
        for (auto key : object.toObject().keys()) {
            if (is_comment_key(key)) {
                continue;
            }
            QJsonValue val = object.toObject()[key];
            if (val.isBool() || val.isDouble() || val.isString()) {
                DependencyValue value{};
                value.from_json(val, true);
                tags.insert(key, value);
            } else if (val.isArray()) {
                for (const auto &item : val.toArray()) {
                    DependencyValue value{};
                    value.from_json(item, true);
                    tags.insert(key, value);
                }
            } else {
                throw std::runtime_error(QString("invalid type of tag description in json file").toStdString());
            }
        }
    } else if (object.isUndefined()) {
        // nothing to do
    } else {
        throw std::runtime_error(QString("invalid tag description in json file").toStdString());
    }
}

#if 0
void DependencyRanges::from_json(const QJsonValue &object, const bool default_to_match_all, const QString &version_key_name) {
    ranges.clear();
    if (object.isArray()) {
        for (auto version_string : object.toArray()) {
            DependencyValue version;
            version.from_json(version_string, default_to_match_all);
            ranges.insertMulti(version_key_name, version);
        }
    } else if (object.isObject()) {
        assert(version_key_name == ""); //avoid recursion
        const QJsonObject &obj = object.toObject();
        auto sl = obj.keys();
        for (auto &s : sl) {
            from_json(obj[s], default_to_match_all, s);
        }
    } else {
        DependencyValue version;
        version.from_json(object, default_to_match_all);
        ranges.insertMulti(version_key_name, version);
    }
}
#endif

DependencyValue::DependencyValue() {
    match_exactly.setValue<double>(0);
    range_high_excluding = 0;
    range_low_including = 0;
    match_style = Match_style::MatchNone;
}

void DependencyValue::from_json(const QJsonValue &object, const bool default_to_match_all) {
    if (object.isString()) {
        from_string(object.toString());
    } else if (object.isDouble()) {
        from_number(object.toDouble());
    } else if (object.isBool()) {
        from_bool(object.toBool());
    } else if (object.isNull()) {
        if (default_to_match_all) {
            match_style = Match_style::MatchEverything;
        } else {
            match_style = Match_style::MatchNone;
        }
    } else if (object.isUndefined()) {
        if (default_to_match_all) {
            match_style = Match_style::MatchEverything;
        } else {
            match_style = Match_style::MatchNone;
        }
    } else {
        throw std::runtime_error(QString("invalid version dependency in json file").toStdString());
    }
}

bool DependencyValue::is_matching(const QVariant &test_value) const {
    const double PRECISION_REDUCTION = 1000 * 1000 * 10;
    switch (match_style) {
        case MatchExactly: {
            bool result = true;
            if ((test_value.type() == QVariant::Double) && (match_exactly.type() == QVariant::Double)) {
                bool ok_a, ok_b;
                double num_val_a = test_value.toDouble(&ok_a);
                double num_val_b = match_exactly.toDouble(&ok_b);
                if (std::round(num_val_a * PRECISION_REDUCTION) == std::round(num_val_b * PRECISION_REDUCTION)) {
                    result = true;
                }
            } else if ((test_value.type() == QVariant::Bool) && (match_exactly.type() == QVariant::Bool)) {
                result = test_value.toBool() == match_exactly.toBool();
            } else {
                result = match_exactly == test_value;
            }
            return result;
        }
        case MatchByRange: {
            bool ok;
            double num_val = test_value.toDouble(&ok);

            double val_a = std::round(num_val * PRECISION_REDUCTION);
            double val_high_excl = std::round(range_high_excluding * PRECISION_REDUCTION);
            double val_low_incl = std::round(range_low_including * PRECISION_REDUCTION);
            assert(ok);

            bool result = (val_low_incl <= val_a);
            if (almost_equal(val_low_incl, val_a, 10)) {
                result = true;
            }

            result = result && (val_a < val_high_excl);

            if (val_a == val_high_excl) {
                result = false;
            }

            return result;
        }
        case MatchEverything:
            return true;

        case MatchNone:
            return false;
    }
}

void DependencyValue::parse_number(const QString &str, float &vnumber, bool &matcheverything) {
    if (str.trimmed() == "*") {
        matcheverything = true;
    } else {
        matcheverything = false;
        bool ok;
        vnumber = str.trimmed().toFloat(&ok);
        if (!ok) {
            throw DataEngineError(DataEngineErrorNumber::invalid_version_dependency_string,
                                  QString("could not parse dependency string \"%1\" in as range number").arg(str));
        }
    }
}

void DependencyValue::from_string(const QString &str) {
    QString value = str.trimmed();
    if (value.startsWith("[") && value.endsWith("]")) {
        value = value.remove(0, 1);
        value = value.remove(value.size() - 1, 1);
        QStringList sl = value.split("-");
        if ((sl.count() == 0) || (value == "")) {
            match_style = Match_style::MatchEverything;
        } else if (sl.count() == 1) {
            float vnumber = 0;
            bool matcheverything = false;
            parse_number(sl[0], vnumber, matcheverything);
            if (matcheverything) {
                match_style = Match_style::MatchEverything;
            } else {
                match_style = Match_style::MatchExactly;
                match_exactly.setValue<double>(vnumber);
            }

        } else if (sl.count() == 2) {
            match_style = Match_style::MatchByRange;
            float vnumber = 0;
            bool matcheverything = false;

            parse_number(sl[0], vnumber, matcheverything);
            if (matcheverything) {
                range_low_including = std::numeric_limits<float>::lowest();
            } else {
                range_low_including = vnumber;
            }

            parse_number(sl[1], vnumber, matcheverything);
            if (matcheverything) {
                range_high_excluding = std::numeric_limits<float>::max();
            } else {
                range_high_excluding = vnumber;
            }

        } else {
            throw DataEngineError(DataEngineErrorNumber::invalid_version_dependency_string,
                                  QString("invalid version dependency string \"%1\"in json file").arg(str));
        }
    } else {
        if (value == "") {
            match_style = Match_style::MatchNone;
        } else {
            match_style = Match_style::MatchExactly;
        }
        match_exactly.setValue<QString>(str);
    }
}

void DependencyValue::from_number(const double &number) {
    match_style = Match_style::MatchExactly;
    match_exactly.setValue<double>(number);
    range_low_including = 0;
    range_high_excluding = 0;
}

void DependencyValue::from_bool(const bool &boolean) {
    match_style = Match_style::MatchExactly;
    match_exactly.setValue<bool>(boolean);
    range_low_including = 0;
    range_high_excluding = 0;
}

bool Data_engine::is_complete(const QMap<QString, QVariant> &tags) const {
    return sections.is_complete(tags);
}

bool Data_engine::all_values_in_range(const QMap<QString, QVariant> &tags) const {
    return sections.all_values_in_range(tags);
}

bool Data_engine::value_in_range(const FormID &id, const QMap<QString, QVariant> &tags) const {
    const DataEngineDataEntry *data_entry = sections.get_entry(id, tags);
    if (data_entry == nullptr) {
        return false;
    }
    return data_entry->is_in_range();
}

void Data_engine::set_actual_number(const FormID &id, const QMap<QString, QVariant> &tags, double number) {
    DataEngineDataEntry *data_entry = sections.get_entry(id, tags);
    assert(data_entry); //if field is not found an exception was already thrown above. Something bad must happen to assert here

    auto number_entry = data_entry->as<NumericDataEntry>();
    if (!number_entry) {
        throw DataEngineError(DataEngineErrorNumber::setting_desired_value_with_wrong_type,
                              QString("The field \"%1\" is not numerical as it must be if you set it with the number (%2) ").arg(id).arg(number));
    }

    number_entry->actual_value = number;
}

void Data_engine::set_actual_text(const FormID &id, const QMap<QString, QVariant> &tags, QString text) {
    DataEngineDataEntry *data_entry = sections.get_entry(id, tags);
    assert(data_entry); //if field is not found an exception was already thrown above. Something bad must happen to assert here

    auto text_entry = data_entry->as<TextDataEntry>();
    if (!text_entry) {
        throw DataEngineError(DataEngineErrorNumber::setting_desired_value_with_wrong_type,
                              QString("The field \"%1\" is not a string as it must be if you set it with the string \"%2\"").arg(id).arg(text));
    }

    text_entry->actual_value = text;
}

void Data_engine::set_actual_bool(const FormID &id, const QMap<QString, QVariant> &tags, bool value) {
    DataEngineDataEntry *data_entry = sections.get_entry(id, tags);
    assert(data_entry); //if field is not found an exception was already thrown above. Something bad must happen to assert here

    auto bool_entry = data_entry->as<BoolDataEntry>();
    if (!bool_entry) {
        throw DataEngineError(DataEngineErrorNumber::setting_desired_value_with_wrong_type,
                              QString("The field \"%1\" is not boolean as it must be if you set it with a bool type").arg(id));
    }

    bool_entry->actual_value = value;
}

double Data_engine::get_desired_value(const FormID &id, const QMap<QString, QVariant> &tags) const {
    const DataEngineDataEntry *data_entry = sections.get_entry(id, tags);
    assert(data_entry);
    return data_entry->as<NumericDataEntry>()->desired_value.value_or(std::numeric_limits<double>::quiet_NaN());
}

QString Data_engine::get_desired_value_as_text(const FormID &id, const QMap<QString, QVariant> &tags) const {
    const DataEngineDataEntry *data_entry = sections.get_entry(id, tags);
    assert(data_entry);
    return data_entry->as<NumericDataEntry>()->get_desired_value_as_string();
}

const QString &Data_engine::get_unit(const FormID &id, const QMap<QString, QVariant> &tags) const {
    const DataEngineDataEntry *data_entry = sections.get_entry(id, tags);
    assert(data_entry);
    return data_entry->as<NumericDataEntry>()->unit;
}

std::unique_ptr<QWidget> Data_engine::get_preview() const {
    QtRPT report;
    fill_report(report, "test.xml");
    report.printExec();
    return nullptr;
}

void Data_engine::generate_pdf(const std::string &form, const std::string &destination) const {
    QtRPT report;
    fill_report(report, QString::fromStdString(form));
    report.printPDF(QString::fromStdString(destination));
}

bool Data_engine::entry_compare(const FormIdWrapper &lhs, const FormIdWrapper &rhs) {
    return lhs.value < rhs.value;
}

void Data_engine::setValue(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage) const {
#if 0
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
#endif
}

void Data_engine::fill_report(QtRPT &report, const QString &form) const {
    if (report.loadReport(form) == false) {
        throw std::runtime_error("Failed opening form file " + form.toStdString());
    }
#if 0
    report.recordCount << data_entries.size();
#endif
    QObject::connect(
        &report, Utility::pick_overload<const int, const QString, QVariant &, const int>(&QtRPT::setValue),
        [this](const int recNo, const QString paramName, QVariant &paramValue, const int reportPage) { setValue(recNo, paramName, paramValue, reportPage); });
}

bool NumericTolerance::test_in_range(const double desired, const std::experimental::optional<double> &measured) const {
    double min_value_absolute = 0;
    double max_value_absolute = 0;

    switch (tolerance_type) {
        case ToleranceType::Absolute: {
            max_value_absolute = desired + deviation_limit_above;
            min_value_absolute = desired - deviation_limit_beneath;
        } break;
        case ToleranceType::Percent: {
            max_value_absolute = desired + desired * deviation_limit_above / 100.0;
            min_value_absolute = desired - desired * deviation_limit_beneath / 100.0;
        }

        break;
    }

    if (open_range_above) {
        max_value_absolute = std::numeric_limits<double>::max();
    }
    if (open_range_beneath) {
        min_value_absolute = std::numeric_limits<double>::lowest();
    }
    return (min_value_absolute <= measured) && (measured <= max_value_absolute);
}

void NumericTolerance::from_string(const QString &str) {
    QStringList sl = str.split("/");
    if (sl.count() == 0) {
        throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error, "no tolerance found");
    }
    if (sl.count() == 1) {
        bool open_range;
        str_to_num(str, deviation_limit_above, tolerance_type, open_range, {"+-", ""});
        deviation_limit_beneath = deviation_limit_above;
        open_range_above = open_range;
        open_range_beneath = open_range;
        is_undefined = false;
    } else if (sl.count() == 2) {
        ToleranceType tolerance_type_a;
        ToleranceType tolerance_type_b;
        bool open_range_a;
        bool open_range_b;
        str_to_num(sl[0], deviation_limit_above, tolerance_type_a, open_range_a, {"+"});
        str_to_num(sl[1], deviation_limit_beneath, tolerance_type_b, open_range_b, {"-"});
        if (open_range_a || open_range_b) {
            tolerance_type = ToleranceType::Absolute;
            if (tolerance_type_a == ToleranceType::Percent) {
                tolerance_type = ToleranceType::Percent;
            }
            if (tolerance_type_b == ToleranceType::Percent) {
                tolerance_type = ToleranceType::Percent;
            }
        } else if (tolerance_type_a == tolerance_type_b) {
            tolerance_type = tolerance_type_a;
        } else {
            throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error, "tolerance + and - must of the same type(either absolute or percent)");
        }
        open_range_above = open_range_a;
        open_range_beneath = open_range_b;
        is_undefined = false;
    } else {
        throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error, "tolerance string faulty");
    }
}

QString NumericTolerance::to_string(const double desired_value) const {
    QString result;

    if (open_range_above && open_range_beneath) {
        result = num_to_str(desired_value, ToleranceType::Absolute) + " (±∞)";
    } else if (open_range_beneath) {
        result = "≤ " + num_to_str(desired_value, ToleranceType::Absolute);
        if (deviation_limit_above > 0.0) {
            result += +" (+" + num_to_str(deviation_limit_above, tolerance_type) + ")";
        }
    } else if (open_range_above) {
        result = "≥ " + num_to_str(desired_value, ToleranceType::Absolute);
        if (deviation_limit_beneath > 0.0) {
            result += +" (-" + num_to_str(deviation_limit_beneath, tolerance_type) + ")";
        }

    } else {
        result = num_to_str(desired_value, ToleranceType::Absolute) + " (";
        if (deviation_limit_above == deviation_limit_beneath) {
            //symmetrical
            result += "±" + num_to_str(deviation_limit_above, tolerance_type) + ")";
        } else {
            //asymmetrical
            result += "+" + num_to_str(deviation_limit_above, tolerance_type) + "/-" + num_to_str(deviation_limit_beneath, tolerance_type) + ")";
        }
    }
    return result;
}

void NumericTolerance::str_to_num(QString str_in, double &number, ToleranceType &tol_type, bool &open, const QStringList expected_sign_strings) {
    QString str_in_orig = str_in;
    str_in = str_in.trimmed();
    open = false;
    tol_type = ToleranceType::Absolute;
    number = 0;
    bool sign_error = true;

    for (auto &sign : expected_sign_strings) {
        if ((sign == "") && ((str_in[0].isDigit()) || (str_in[0] == "*"))) {
            sign_error = false;
            break;
        } else if (str_in.startsWith(sign) && (sign != "")) {
            sign_error = false;
            str_in.remove(0, sign.count());
            break;
        }
    }
    if (str_in == "*") {
        open = true;
        sign_error = false;
    } else {
        if (str_in.endsWith("%")) {
            tol_type = ToleranceType::Percent;
            str_in.remove(str_in.count() - 1, 1);
        } else {
            tol_type = ToleranceType::Absolute;
        }
        bool ok;
        number = str_in.toDouble(&ok);
        if (!ok) {
            throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error, QString("could not convert string to number %1").arg(str_in_orig));
        }
    }
    if (sign_error) {
        throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error, QString("tolerance string should have sign flags %1").arg(str_in_orig));
    }
}

QString NumericTolerance::num_to_str(double number, ToleranceType tol_type) const {
    QString numStr;
    numStr = QString::number(number, 'f', 10);
    if (numStr.indexOf(".") > 0) {
        while (numStr.endsWith("0")) {
            numStr.remove(numStr.count() - 1, 1);
        }
        if (numStr.endsWith(".")) {
            numStr.remove(numStr.count() - 1, 1);
        }
    }
    if (tol_type == ToleranceType::Percent) {
        numStr += "%";
    }

    return numStr;
}

std::unique_ptr<DataEngineDataEntry> DataEngineDataEntry::from_json(const QJsonObject &object) {
    FormID field_name;
    EntryType entrytype = EntryType::Unspecified;
    const auto keys = object.keys();
    bool entry_without_value;
    if (!keys.contains("value")) {
        if (!keys.contains("type")) {
            throw DataEngineError(DataEngineErrorNumber::data_entry_contains_neither_type_nor_value, "JSON object must contain a key \"value\" or \"type\" ");
        } else {
            entry_without_value = true;
            QString str_type = object["type"].toString();
            if (str_type == "number") {
                entrytype = EntryType::Numeric;
            } else if (str_type == "string") {
                entrytype = EntryType::String;
            } else if (str_type == "bool") {
                entrytype = EntryType::Bool;
            }
        }
    } else {
        entry_without_value = false;
        auto value = object["value"];
        if (value.isDouble()) {
            entrytype = EntryType::Numeric;
        } else if (value.isString()) {
            entrytype = EntryType::String;
            QString str = value.toString();
            if ((str.startsWith("[")) && (str.endsWith("]"))) {
                entrytype = EntryType::Reference;
            }
        } else if (value.isBool()) {
            entrytype = EntryType::Bool;
        }
    }

    if (!keys.contains("name")) {
        throw DataEngineError(DataEngineErrorNumber::data_entry_contains_no_name, "JSON object must contain key \"name\"");
    }
    field_name = object.value("name").toString();
    switch (entrytype) {
        case EntryType::Numeric: {
            std::experimental::optional<double> desired_value{};
            NumericTolerance tolerance{};
            double si_prefix = 1;
            QString unit{};
            QString nice_name{};

            for (const auto &key : keys) {
                if (key == "name") {
                    //already handled above
                } else if (is_comment_key(key)) {
                    //nothing to do

                } else if (key == "nice_name") {
                    nice_name = object.value(key).toString();

                } else if ((key == "si_prefix")) {
                    si_prefix = object.value(key).toDouble(0.);

                } else if (key == "unit") {
                    unit = object.value(key).toString();
                } else if ((key == "value") && (entry_without_value == false)) {
                    desired_value = object.value(key).toDouble(0.);
                } else if ((key == "type") && (entry_without_value == true)) {
                    //already handled above
                } else if ((key == "tolerance") && (entry_without_value == false)) {
                    QString tol;
                    if (object.value(key).isDouble()) {
                        tol = QString::number(object.value(key).toDouble());
                    } else if (object.value(key).isString()) {
                        tol = object.value(key).toString();
                    } else {
                        throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error, "wrong tolerance type");
                    }

                    tolerance.from_string(tol);
                } else if (entry_without_value == false) {
                    throw DataEngineError(DataEngineErrorNumber::invalid_data_entry_key, "Invalid key \"" + key + "\" in numeric JSON object");

                } else {
                    throw DataEngineError(DataEngineErrorNumber::invalid_data_entry_key,
                                          "Invalid key \"" + key + "\" in numeric JSON object without desired value");
                }
            }
            return std::make_unique<NumericDataEntry>(field_name, desired_value, tolerance, std::move(unit), std::move(nice_name));
        }
        case EntryType::Bool: {
            std::experimental::optional<bool> desired_value{};
            QString nice_name{};
            for (const auto &key : keys) {
                if (key == "name") {
                    //already handled above
                } else if (is_comment_key(key)) {
                    //nothing to do
                } else if (key == "nice_name") {
                    nice_name = object.value(key).toString();
                } else if ((key == "value") && (entry_without_value == false)) {
                    desired_value = object.value(key).toBool();
                } else if ((key == "type") && (entry_without_value == true)) {
                    //already handled above
                } else {
                    throw DataEngineError(DataEngineErrorNumber::invalid_data_entry_key, "Invalid key \"" + key + "\" in boolean JSON object");
                }
            }
            return std::make_unique<BoolDataEntry>(field_name, desired_value);
            break;
        }
        case EntryType::String: {
            std::experimental::optional<QString> desired_value{};
            QString nice_name{};
            for (const auto &key : keys) {
                if (key == "name") {
                    //already handled above
                } else if (is_comment_key(key)) {
                    //nothing to do
                } else if (key == "nice_name") {
                    nice_name = object.value(key).toString();
                } else if ((key == "value") && (entry_without_value == false)) {
                    desired_value = object.value(key).toString();
                } else if ((key == "type") && (entry_without_value == true)) {
                    //already handled above
                } else {
                    throw DataEngineError(DataEngineErrorNumber::invalid_data_entry_key, "Invalid key \"" + key + "\" in textual JSON object");
                }
            }
            return std::make_unique<TextDataEntry>(field_name, desired_value);
        }
        case EntryType::Reference: {
            QString reference_string{};
            QString nice_name{};
            NumericTolerance tolerance{};
            for (const auto &key : keys) {
                if (key == "name") {
                    //already handled above
                } else if (is_comment_key(key)) {
                    //nothing to do
                } else if (key == "nice_name") {
                    nice_name = object.value(key).toString();
                } else if (key == "tolerance") {
                    QString tol;
                    if (object.value(key).isDouble()) {
                        tol = QString::number(object.value(key).toDouble());
                    } else if (object.value(key).isString()) {
                        tol = object.value(key).toString();
                    } else {
                        throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error, "wrong tolerance type");
                    }

                    tolerance.from_string(tol);
                } else if (key == "value") {
                    reference_string = object.value(key).toString();

                } else {
                    throw DataEngineError(DataEngineErrorNumber::invalid_data_entry_key, "Invalid key \"" + key + "\" in reference JSON object");
                }
            }
            return std::make_unique<ReferenceDataEntry>(field_name, reference_string, tolerance);
        }
        case EntryType::Unspecified: {
            throw DataEngineError(DataEngineErrorNumber::invalid_data_entry_type, "Invalid type in JSON object");
        }
    }
    throw DataEngineError(DataEngineErrorNumber::invalid_json_object, "invalid JSON object");
}

NumericDataEntry::NumericDataEntry(FormID field_name, std::experimental::optional<double> desired_value, NumericTolerance tolerance, QString unit,
                                   QString description)
    : DataEngineDataEntry(field_name)
    , desired_value(desired_value)
    , tolerance(tolerance)
    , unit(std::move(unit))
    , description(std::move(description)) {}

bool NumericDataEntry::is_complete() const {
    return bool(actual_value);
}

bool NumericDataEntry::is_in_range() const {
    if (bool(desired_value)) {
        return is_complete() && tolerance.test_in_range(desired_value.value(), actual_value);
    } else {
        return is_complete();
    }
}

QString NumericDataEntry::get_desired_value_as_string() const {
    if (bool(desired_value)) {
        return tolerance.to_string(desired_value.value());
    } else {
        return "";
    }
}

QString NumericDataEntry::get_value() const {
    return is_complete() ? QString::number(actual_value.value()) : unavailable_value;
}

QString NumericDataEntry::get_description() const {
    return description;
}

TextDataEntry::TextDataEntry(const FormID name, std::experimental::optional<QString> desired_value)
    : DataEngineDataEntry(name)
    , desired_value(std::move(desired_value)) {}

bool TextDataEntry::is_complete() const {
    return bool(actual_value);
}

bool TextDataEntry::is_in_range() const {
    if (bool(desired_value)) {
        return is_complete() && actual_value.value() == desired_value;
    } else {
        return is_complete();
    }
}

QString TextDataEntry::get_value() const {
    return is_complete() ? actual_value.value() : unavailable_value;
}

QString TextDataEntry::get_description() const {
    return description;
}

QString TextDataEntry::get_desired_value_as_string() const {
    if (bool(desired_value)) {
        return desired_value.value();
    } else {
        return "";
    }
}

QString Data_engine::Statistics::to_qstring() const {
    const int total = number_of_id_fields + number_of_data_fields;
    return QString(R"(Fields filled: %1/%2
Fields succeeded: %3/%4)")
        .arg(number_of_filled_fields)
        .arg(total)
        .arg(number_of_inrange_fields)
        .arg(total);
}

BoolDataEntry::BoolDataEntry(const FormID name, std::experimental::optional<bool> desired_value)
    : DataEngineDataEntry(name)
    , desired_value(std::move(desired_value)) {}

bool BoolDataEntry::is_complete() const {
    return bool(actual_value);
}

bool BoolDataEntry::is_in_range() const {
    if (bool(desired_value)) {
        return is_complete() && actual_value.value() == desired_value;
    } else {
        return is_complete();
    }
}

QString BoolDataEntry::get_value() const {
    if (is_complete()) {
        if (actual_value.value()) {
            return "true";
        } else {
            return "false";
        }
    } else {
        return unavailable_value;
    }
}

QString BoolDataEntry::get_description() const {
    return description;
}

QString BoolDataEntry::get_desired_value_as_string() const {
    if (bool(desired_value)) {
        if (desired_value.value()) {
            return "true";
        } else {
            return "false";
        }
    } else {
        return "";
    }
}

ReferenceDataEntry::ReferenceDataEntry(const FormID name, QString reference_string, NumericTolerance tolerance)
    : DataEngineDataEntry(name)
    , reference_string(reference_string)
    , tolerance(tolerance){}

bool ReferenceDataEntry::is_complete() const
{
    return true;
}

bool ReferenceDataEntry::is_in_range() const
{
    return true;
}

QString ReferenceDataEntry::get_value() const
{
    return "";
}

QString ReferenceDataEntry::get_description() const
{
    return description;
}

QString ReferenceDataEntry::get_desired_value_as_string() const
{
    return "";
}
