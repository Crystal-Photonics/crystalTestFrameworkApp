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

const DataEngineDataEntry *DataEngineSections::get_entry(const FormID &id, const QMultiMap<QString, QVariant> &tags,
                                                         const QMap<QString, double> &versions_including,
                                                         const QMap<QString, double> &versions_excluding) const {
    auto names = id.split("/");
    if (names.count() != 2) {
        throw DataEngineError(DataEngineErrorNumber::faulty_field_id, QString("field id needs to be in format \"section-name/field-name\" but is %1").arg(id));
    }
    bool section_found = false;
    QString section_name = names[0];
    QString field_name = names[1];
    for (auto &section : sections) {
        if (section.get_section_name() == section_name) {
            const VariantData *variant = section.get_variant(tags, versions_including, versions_excluding);
            assert(variant);

            section_found = true;
            break;
        }
    }
    if (!section_found) {
        throw DataEngineError(DataEngineErrorNumber::no_section_id_found, QString("Could not find section with name = \"%1\"").arg(section_name));
    }
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

const VariantData *DataEngineSection::get_variant(const QMultiMap<QString, QVariant> &tags, const QMap<QString, double> &versions_including,
                                                  const QMap<QString, double> &versions_excluding) const {
    bool matched = false;

    for (const auto &variant : variants) {
        if (variant.is_matching_tags_n_versions(tags, versions_including, versions_excluding)) {
            if (matched) {
                throw;
            }
            const VariantData *result = &variant;
            matched = true;
            return result;
        }
    }
    return nullptr;
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

bool VariantData::is_matching_tags_n_versions(const QMultiMap<QString, QVariant> &tags, const QMap<QString, double> &versions_including_,
                                              const QMap<QString, double> &versions_excluding_) const {
    bool tag_matches = true;
    bool version_including_matches = false;
    bool version_excluding_matches = false;

    for (const auto &tag_key : dependency_tags.tags.keys()) {
        //TODO: wenn in dependency_tags ein wildcat ist, dann nicht beachten.
        if (tags.contains(tag_key)) {
            bool at_least_value_matches = false;
            for (auto &value : dependency_tags.tags.values(tag_key)) {
                //TODO: wenn tags ein wildcat hat, dann at_least_value_matches = true;
                if (tags.contains(tag_key, value)) {
                    at_least_value_matches = true;
                    break;
                }
            }
            if (!at_least_value_matches) {
                tag_matches = false;
            }
        } else {
            tag_matches = false;
        }
    }

    bool containing_include = true;
    bool matching_include = false;
    for (const auto &versions_name : versions_excluding.versions.keys()) {
        if (!versions_including_.contains(versions_name)) {
            containing_include = false;
            break;
        }
        double version = versions_including_[versions_name];
        for (auto &value : versions_excluding.versions.values(versions_name)) {
            if (value.is_matching(version)) {
                matching_include = true;
                break;
            }
        }
    }
    version_including_matches = containing_include && matching_include;
}

void VariantData::from_json(const QJsonObject &object) {
    auto legal_versions_j = object["legal_versions"];
    auto illlegal_versions_j = object["illlegal_versions"];
    auto tags_j = object["tags"];
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
    }

    versions_including.from_json(legal_versions_j, true, "");
    versions_excluding.from_json(illlegal_versions_j, false, "");
    dependency_tags.from_json(tags_j);
}

bool VariantData::value_exists(QString field_name) {
    bool result = false;
    for (const auto &entry : data_entries) {
        if (entry.get()->field_name == field_name) {
            result = true;
            break;
        }
    }
    return result;
}

void DependencyTags::from_json(const QJsonValue &object) {
    tags.clear();
    if (object.isObject()) {
        for (auto key : object.toObject().keys()) {
            if (is_comment_key(key)) {
                continue;
            }
            QVariant value;
            QJsonValue val = object.toObject()[key];
            if (val.isBool() || val.isDouble() || val.isString()) {
                value = val.toVariant();
            } else {
                throw std::runtime_error(QString("invalid type of tag description in json file").toStdString());
            }
            tags.insert(key, value);
        }
    } else if (object.isUndefined()) {
        // nothing to do
    } else {
        throw std::runtime_error(QString("invalid tag description in json file").toStdString());
    }
}

void DependencyVersions::from_json(const QJsonValue &object, const bool default_to_match_all, const QString &version_key_name) {
    versions.clear();
    if (object.isArray()) {
        for (auto version_string : object.toArray()) {
            DependencyVersion version;
            version.from_json(version_string, default_to_match_all);
            versions.insertMulti(version_key_name, version);
        }
    } else if (object.isObject()) {
        assert(version_key_name == ""); //avoid recursion
        const QJsonObject &obj = object.toObject();
        auto sl = obj.keys();
        for (auto &s : sl) {
            from_json(obj[s], default_to_match_all, s);
        }
    } else {
        DependencyVersion version;
        version.from_json(object, default_to_match_all);
        versions.insertMulti(version_key_name, version);
    }
}

DependencyVersion::DependencyVersion() {
    version_number_match_exactly = 0;
    version_number_low_including = 0;
    version_number_high_excluding = 0;
    match_style = Match_style::MatchNone;
}

void DependencyVersion::from_json(const QJsonValue &object, const bool default_to_match_all) {
    if (object.isString()) {
        from_string(object.toString());
    } else if (object.isDouble()) {
        from_number(object.toDouble());
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

bool DependencyVersion::is_matching(float version_number) {
    switch (match_style) {
        case MatchExactly:
            return version_number_match_exactly == version_number;

        case MatchByRange:
            return (version_number_low_including <= version_number) && (version_number < version_number_high_excluding);

        case MatchEverything:
            return true;

        case MatchNone:
            return false;
    }
}

void DependencyVersion::parse_version_number(const QString &str, float &vnumber, bool &matcheverything) {
    if (str.trimmed() == "*") {
        matcheverything = true;
    } else {
        matcheverything = false;
        bool ok;
        vnumber = str.trimmed().toFloat(&ok);
        if (!ok) {
            throw DataEngineError(DataEngineErrorNumber::invalid_version_dependency_string,
                                  QString("could not parse version dependency string \"%1\" in as number").arg(str));
        }
    }
}

void DependencyVersion::from_string(const QString &str) {
    QStringList sl = str.split("-");
    if ((sl.count() == 0) || (str == "")) {
        match_style = Match_style::MatchEverything;
    } else if (sl.count() == 1) {
        float vnumber = 0;
        bool matcheverything = false;
        parse_version_number(sl[0], vnumber, matcheverything);
        if (matcheverything) {
            match_style = Match_style::MatchEverything;
        } else {
            match_style = Match_style::MatchExactly;
            version_number_match_exactly = vnumber;
        }

    } else if (sl.count() == 2) {
        match_style = Match_style::MatchByRange;
        float vnumber = 0;
        bool matcheverything = false;

        parse_version_number(sl[0], vnumber, matcheverything);
        if (matcheverything) {
            version_number_low_including = std::numeric_limits<float>::lowest();
        } else {
            version_number_low_including = vnumber;
        }

        parse_version_number(sl[1], vnumber, matcheverything);
        if (matcheverything) {
            version_number_high_excluding = std::numeric_limits<float>::max();
        } else {
            version_number_high_excluding = vnumber;
        }

    } else {
        throw DataEngineError(DataEngineErrorNumber::invalid_version_dependency_string,
                              QString("invalid version dependency string \"%1\"in json file").arg(str));
    }
}

void DependencyVersion::from_number(const double &version_number) {
    match_style = Match_style::MatchExactly;
    version_number_match_exactly = version_number;
    version_number_low_including = 0;
    version_number_high_excluding = 0;
}

bool Data_engine::is_complete() const {
    return true;
}

bool Data_engine::all_values_in_range() const {
    return true;
}

bool Data_engine::value_in_range(const FormID &id) const {
#if 0
    auto entry = get_entry(id);
    if (entry == nullptr) {
        return false;
    }
    return entry->is_in_range();
#else
    return true;
#endif
}

void Data_engine::set_actual_number(const FormID &id, double number) {
#if 0
    auto entry = get_entry(id);
    if (entry == nullptr) {
        qDebug() << "Tried setting invalid field" << id;
        return;
    }
    auto number_entry = entry->as<NumericDataEntry>();
    if (number_entry == nullptr) {
        qDebug() << "Tried setting number to non-number field" << id;
        return;
    }
    number_entry->actual_value = number;
#endif
}

void Data_engine::set_actual_text(const FormID &id, QString text) {
#if 0
    get_entry(id)->as<TextDataEntry>()->actual_value = std::move(text);
#endif
}

double Data_engine::get_desired_value(const FormID &id, const QMultiMap<QString, QVariant> &tags, const QMap<QString, double> &versions_including,
                                      const QMap<QString, double> &versions_excluding) const {
    const DataEngineDataEntry *data_entry = sections.get_entry(id, tags, versions_including, versions_including);
    assert(data_entry);
    return data_entry->as<NumericDataEntry>()->target_value;
}

double Data_engine::get_desired_absolute_tolerance(const FormID &id) const {
    return 0;
    //return get_entry(id)->as<NumericDataEntry>()->tolerance;
}

double Data_engine::get_desired_relative_tolerance(const FormID &id) const {
    return 0;
    //return get_entry(id)->as<NumericDataEntry>()->tolerance / get_entry(id)->as<NumericDataEntry>()->target_value;
}

double Data_engine::get_desired_minimum(const FormID &id) const {
    return 0;
    //  return get_entry(id)->as<NumericDataEntry>()->target_value - get_entry(id)->as<NumericDataEntry>()->tolerance;
}

double Data_engine::get_desired_maximum(const FormID &id) const {
    return 0;
    //return get_entry(id)->as<NumericDataEntry>()->target_value + get_entry(id)->as<NumericDataEntry>()->tolerance;
}

const QString &Data_engine::get_unit(const FormID &id) const {
#if 0
    return get_entry(id)->as<NumericDataEntry>()->unit;
#else
    return "";
#endif
}

#if 0
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
#endif
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

void Data_engine::add_entry(std::pair<FormID, std::unique_ptr<DataEngineDataEntry>> &&entry) {
    auto debug = &entry;
#if 0
    if (entry.first.isEmpty()) {
        auto pos = std::lower_bound(std::begin(data_entries), std::end(data_entries), entry.second, entry_compare);
        data_entries.insert(pos, std::move(entry.second));
    } else {
        auto pos = std::lower_bound(std::begin(id_entries), std::end(id_entries), entry, entry_compare);
        id_entries.insert(pos, std::move(entry));
    }
#endif
}

DataEngineDataEntry *Data_engine::get_entry(const FormID &id, const QMultiMap<QString, QVariant> &tags, const QMap<QString, double> &versions_including,
                                            const QMap<QString, double> &versions_excluding) {
    return const_cast<DataEngineDataEntry *>(Utility::as_const(*this).get_entry(id, tags, versions_including, versions_excluding));
}

const DataEngineDataEntry *Data_engine::get_entry(const FormID &id, const QMultiMap<QString, QVariant> &tags, const QMap<QString, double> &versions_including,
                                                  const QMap<QString, double> &versions_excluding) const {
#if 0
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
        if (pos != std::end(data_entries)) {
            auto desc = pos->get()->get_description();
            if (desc == id) {
                return pos->get();
            }
        }
        return nullptr;
    }
#else
    return nullptr;

#endif
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

    } else {
        throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error, "tolerance string faulty");
    }
}

QString NumericTolerance::to_string(const double desired_value) {
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

QString NumericTolerance::num_to_str(double number, ToleranceType tol_type) {
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
    EntryType entrytype;
    const auto keys = object.keys();
    if (!keys.contains("value")) {
        if (!keys.contains("type")) {
            throw std::runtime_error("JSON object must contain a key \"value\" or \"type\" ");
        } else {
            QString str_type = object["type"].toString();
            if (str_type == "numeric") {
                entrytype = EntryType::Numeric;
            } else if (str_type == "string") {
                entrytype = EntryType::String;
            } else if (str_type == "bool") {
                entrytype = EntryType::Bool;
            }
        }
    } else {
        auto value = object["value"];
        if (value.isDouble()) {
            entrytype = EntryType::Numeric;
        } else if (value.isString()) {
            entrytype = EntryType::String;
        } else if (value.isBool()) {
            entrytype = EntryType::Bool;
        }
    }

    if (!keys.contains("name")) {
        throw std::runtime_error("JSON object must contain key \"name\"");
    }
    field_name = object.value("name").toString();
    switch (entrytype) {
        case EntryType::Numeric: {
            double target_value{};
            NumericTolerance tolerance{};
            double si_prefix = 1;
            QString unit{};
            QString nice_name{};

            for (const auto &key : keys) {
                if (key == "name") {
                    //already handled above

                } else if (key == "si_prefix") {
                } else if (key == "nice_name") {
                    nice_name = object.value(key).toString();

                } else if (key == "si_prefix") {
                    si_prefix = object.value(key).toDouble(0.);
                } else if (key == "unit") {
                    unit = object.value(key).toString();
                } else if (key == "value") {
                    target_value = object.value(key).toDouble(0.); //put error here if not convertable
                } else if (key == "tolerance") {
                    QString tol;
                    if (object.value(key).isDouble()) {
                        tol = QString::number(object.value(key).isDouble());
                    } else if (object.value(key).isString()) {
                        tol = object.value(key).toString();
                    } else {
                        throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error, "wrong tolerance type");
                    }

                    tolerance.from_string(tol);
                } else {
                    throw std::runtime_error("Invalid key \"" + key.toStdString() + "\" in numeric JSON object");
                }
            }
            return std::make_unique<NumericDataEntry>(field_name, target_value, tolerance, std::move(unit), std::move(nice_name));
        }
        case EntryType::Bool: {
            break;
        }
        case EntryType::String: {
            QString target_value{};
            QString nice_name{};
            for (const auto &key : keys) {
                if (key == "name") {
                    //already handled above

                } else if (key == "nice_name") {
                    nice_name = object.value(key).toString();
                } else if (key == "value") {
                    target_value = object.value(key).toString();
                } else {
                    throw std::runtime_error("Invalid key \"" + key.toStdString() + "\" in textual JSON object");
                }
            }
            return std::make_unique<TextDataEntry>(field_name, target_value);
        }
    }
    throw std::runtime_error("invalid JSON object");
}

NumericDataEntry::NumericDataEntry(FormID field_name, double target_value, NumericTolerance tolerance, QString unit, QString description)
    : target_value(target_value)
    , tolerance(tolerance)
    , unit(std::move(unit))
    , description(std::move(description)) {}

bool NumericDataEntry::is_complete() const {
    return bool(actual_value);
}

bool NumericDataEntry::is_in_range() const {
    return true;
    //return is_complete() && std::abs(actual_value.value() - target_value) <= tolerance;
}

QString NumericDataEntry::get_value() const {
    return is_complete() ? QString::number(actual_value.value()) : unavailable_value;
}

QString NumericDataEntry::get_description() const {
    return description;
}

QString NumericDataEntry::get_minimum() const {
    return QString::number(get_min_value());
}

QString NumericDataEntry::get_maximum() const {
    return QString::number(get_max_value());
}

double NumericDataEntry::get_min_value() const {
    return 0;
    //return target_value - tolerance;
}

double NumericDataEntry::get_max_value() const {
    return 0;
    //return target_value + tolerance;
}

TextDataEntry::TextDataEntry(FormID name, QString target_value)
    : target_value(std::move(target_value)) {}

bool TextDataEntry::is_complete() const {
    return bool(actual_value);
}

bool TextDataEntry::is_in_range() const {
    return is_complete() && actual_value.value() == target_value;
}

QString TextDataEntry::get_value() const {
    return is_complete() ? actual_value.value() : unavailable_value;
}

QString TextDataEntry::get_description() const {
    return description;
}

QString TextDataEntry::get_minimum() const {
    return "";
}

QString TextDataEntry::get_maximum() const {
    return "";
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
