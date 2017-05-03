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

Data_engine::Data_engine(std::istream &source, const QMap<QString, QVariant> &tags) {
    set_source(source, tags);
}

void Data_engine::set_source(std::istream &source, const QMap<QString, QVariant> &tags) {
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
    sections.delete_unmatched_variants(tags);
    sections.deref_references();
    sections.create_already_defined_instances();
}

void DataEngineSections::delete_unmatched_variants(const QMap<QString, QVariant> &tags) {
    for (auto &section : sections) {
        section.delete_unmatched_variants(tags);
        if ((section.is_allow_empty_section()) && (section.variants.size() == 0)){
            VariantData variant_data{};
            section.variants.push_back(std::move(variant_data));

        }
    }
}

void DataEngineSections::deref_references() {
    for (auto &section : sections) {
        if (section.variants.size() == 0) {
            continue;
        }
        const VariantData *variant_to_test = section.get_variant();
        for (const auto &entry : variant_to_test->data_entries) {
            ReferenceDataEntry *reference = entry.get()->as<ReferenceDataEntry>();
            if (reference) {
                reference->dereference(this);
            }
        }
    }
}

void DataEngineSections::set_instance_count(QString instance_count_name, uint instance_count) {
    bool instance_count_name_found = false;
    for (auto &section : sections) {
        if (section.set_instance_count_if_name_matches(instance_count_name, instance_count)) {
            instance_count_name_found = true;
        }
    }
    if (instance_count_name_found == false) {
        throw DataEngineError(DataEngineErrorNumber::instance_count_does_not_exist,
                              QString("Could not find instance count name:\"%1\"").arg(instance_count_name));
    }
}

void DataEngineSections::use_instance(QString section_name, QString instance_caption, uint instance_index) {
    DataEngineErrorNumber error_num = DataEngineErrorNumber::ok;
    DataEngineSection *section_to_use = get_section_raw(section_name, &error_num);

    if (error_num == DataEngineErrorNumber::no_section_id_found) {
        throw DataEngineError(DataEngineErrorNumber::no_section_id_found, QString("Could not find section with name = \"%1\"").arg(section_name));
    }
    assert(section_to_use);
    section_to_use->use_instance(instance_caption, instance_index);
}

void DataEngineSections::create_already_defined_instances() {
    for (auto &section : sections) {
        section.create_instances_if_defined();
        if (section.is_section_instance_defined()) {
            section.use_instance("", 1);
        }
    }
}

DataEngineSection *DataEngineSections::get_section_raw(const QString &section_name, DataEngineErrorNumber *error_num) const {
    const DataEngineSection *result = nullptr;
    *error_num = DataEngineErrorNumber::ok;
    for (auto &section : sections) {
        if (section.get_section_name() == section_name) {
            assert(result == nullptr); //there should only be one section with this name
            result = &section;
        }
    }
    if (result == nullptr) {
        *error_num = DataEngineErrorNumber::no_section_id_found;
    }
    return const_cast<DataEngineSection *>(result);
}

const DataEngineDataEntry *DataEngineSections::get_entry_raw(const FormID &id, DataEngineErrorNumber *error_num, QString &section_name,
                                                             QString &field_name) const {
    auto names = id.split("/");
    if (names.count() != 2) {
        throw DataEngineError(DataEngineErrorNumber::faulty_field_id, QString("field id needs to be in format \"section-name/field-name\" but is %1").arg(id));
    }
    *error_num = DataEngineErrorNumber::ok;
    bool section_found = false;
    section_name = names[0];
    field_name = names[1];
    DataEngineSection *section = get_section_raw(section_name, error_num);

    if (section) {
        const VariantData *variant = section->get_variant();
        assert(variant);
        section_found = true;

        if (section->is_section_instance_defined() == false) {
            section_found = false;
            throw DataEngineError(DataEngineErrorNumber::instance_count_yet_undefined,
                                  QString("Instance count of section \"%1\" yet undefined. Instance count value is: \"%2\".")
                                      .arg(section->get_section_name())
                                      .arg(section->get_instance_count_name()));
        }

        const DataEngineDataEntry *result = variant->get_value(field_name);
        if (result == nullptr) {
            *error_num = DataEngineErrorNumber::no_field_id_found;
        }
        return result;
    }

    return nullptr;
}

const DataEngineDataEntry *DataEngineSections::get_entry(const FormID &id) const {
    DataEngineErrorNumber error_num = DataEngineErrorNumber::ok;
    QString field_name;
    QString section_name;
    const DataEngineDataEntry *result = get_entry_raw(id, &error_num, section_name, field_name);
    if (error_num == DataEngineErrorNumber::no_field_id_found) {
        throw DataEngineError(DataEngineErrorNumber::no_field_id_found, QString("Could not find field with name = \"%1\"").arg(field_name));
    } else if (error_num == DataEngineErrorNumber::no_section_id_found) {
        throw DataEngineError(DataEngineErrorNumber::no_section_id_found, QString("Could not find section with name = \"%1\"").arg(section_name));
    } else if (error_num != DataEngineErrorNumber::ok) {
        assert(0);
    }

    return result;
}

DataEngineDataEntry *DataEngineSections::get_entry(const FormID &id) {
    return const_cast<DataEngineDataEntry *>(Utility::as_const(*this).get_entry(id));
}

bool DataEngineSections::exists_uniquely(const FormID &id) const {
    DataEngineErrorNumber error_num = DataEngineErrorNumber::ok;
    QString field_name;
    QString section_name;
    bool result = true;
    const DataEngineDataEntry *ptr = get_entry_raw(id, &error_num, section_name, field_name);
    if (error_num == DataEngineErrorNumber::no_field_id_found) {
        result = false;
    } else if (error_num == DataEngineErrorNumber::no_section_id_found) {
        result = false;
    } else if (error_num != DataEngineErrorNumber::ok) {
        assert(0);
        result = false;
    }
    result = result && (ptr != nullptr);

    return result;
}

bool DataEngineSections::is_complete() const {
    bool result = true;
    for (const auto &section : sections) {
        if (!section.is_complete()) {
            result = false;
        }
    }
    return result;
}

bool DataEngineSections::all_values_in_range() const {
    bool result = true;
    for (const auto &section : sections) {
        if (!section.all_values_in_range()) {
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
    DataEngineErrorNumber error_num;
    get_section_raw(section_name, &error_num);
    return error_num == DataEngineErrorNumber::ok;
}

void DataEngineSection::delete_unmatched_variants(const QMap<QString, QVariant> &tags) {
    uint i = 0;
    while (i < variants.size()) {
        auto &variant = variants[i];
        if (variant.is_dependency_matching(tags)) {
            i++;
        } else {
            variants.erase(variants.begin() + i);
        }
    }
    if (variants.size() > 1) {
        throw DataEngineError(DataEngineErrorNumber::non_unique_desired_field_found,
                              QString("More than one dependency fullfilling variants (%1) found in section: \"%2\"").arg(variants.size()).arg(section_name));
    }
}

const VariantData *DataEngineSection::get_variant() const {
    if (variants.size() == 1) {
        return &variants[0];
    }

    throw DataEngineError(DataEngineErrorNumber::no_variant_found, QString("No dependency fullfilling variant found in section: \"%1\"").arg(section_name));
    return nullptr;
}

bool DataEngineSection::is_complete() const {
    bool result = true;
    const VariantData *variant_to_test = get_variant();
    assert(variant_to_test);
    for (const auto &entry : variant_to_test->data_entries) {
        if (!entry.get()->is_complete()) {
            result = false;
        }
    }
    return result;
}

bool DataEngineSection::all_values_in_range() const {
    bool result = true;
    const VariantData *variant_to_test = get_variant();
    for (const auto &entry : variant_to_test->data_entries) {
        if (!entry.get()->is_in_range()) {
            result = false;
        }
    }
    return result;
}
QString jsonTypeToString(const QJsonValue &val) {
    switch (val.type()) {
        case QJsonValue::Null:
            return "Null";
            break;
        case QJsonValue::Bool:
            return "Bool";
            break;
        case QJsonValue::Double:
            return "Double";
            break;
        case QJsonValue::String:
            return "String";
            break;
        case QJsonValue::Array:
            return "Array";
            break;
        case QJsonValue::Object:
            return "Object";
            break;
        case QJsonValue::Undefined:
            return "Undefined";
            break;
    }
}

void DataEngineSection::from_json(const QJsonValue &object, const QString &key_name) {
    section_name = key_name;
    if (object.isArray()) {
        for (auto variant : object.toArray()) {
            const QJsonObject &obj = variant.toObject();
            if (obj.contains("instance_count")) {
                throw DataEngineError(DataEngineErrorNumber::instance_count_must_not_be_defined_in_variant_scope,
                                      QString("Instance count of section \"%1\" must not be a defined within variant scope.").arg(get_section_name()));
            }
            if (obj.contains("allow_empty_section")) {
                throw DataEngineError(DataEngineErrorNumber::allow_empty_section_must_not_be_defined_in_variant_scope,
                                      QString("Instance count of section \"%1\" must not be a defined within variant scope.").arg(get_section_name()));
            }
            append_variant_from_json(obj);
        }
        instance_count = 1;
        instance_count_name = "";
    } else if (object.isObject()) {
        QJsonObject obj = object.toObject();
        if (obj.contains("instance_count")) {
            bool is_string = obj["instance_count"].isString();
            bool is_number = obj["instance_count"].isDouble();
            double instance_count_dbl_json = 0;
            if ((!is_string) && (!is_number)) {
                throw DataEngineError(DataEngineErrorNumber::wrong_type_for_instance_count,
                                      QString("Instance count of section \"%1\" must not be either integer or string. But is \"%2\".")
                                          .arg(get_section_name())
                                          .arg(jsonTypeToString(obj["instance_count"])));
            }
            if (is_string) {
                QString instance_count_name_lcl = obj["instance_count"].toString();
                bool ok = true;
                double instance_count_dbl = instance_count_name_lcl.toDouble(&ok);

                if (!ok) {
                    //is string -> alles ok
                    instance_count_name = instance_count_name_lcl;

                } else {
                    is_string = false;
                    is_number = true;
                    instance_count_dbl_json = instance_count_dbl;
                }
            } else {
                instance_count_dbl_json = obj["instance_count"].toDouble();
            }
            if (is_number) {
                instance_count_name = "";
                instance_count = instance_count_dbl_json;

                if (round(instance_count_dbl_json) != instance_count_dbl_json) {
                    throw DataEngineError(DataEngineErrorNumber::instance_count_must_not_be_zero_nor_fraction_nor_negative,
                                          QString("Instance count of section \"%1\" must not be a fraction.").arg(get_section_name()));
                }

                if (instance_count_dbl_json < 0) {
                    throw DataEngineError(DataEngineErrorNumber::instance_count_must_not_be_zero_nor_fraction_nor_negative,
                                          QString("Instance count of section \"%1\" must not be negative.").arg(get_section_name()));
                }
            }

        } else {
            instance_count = 1;
            instance_count_name = "";
        }
        if (obj.contains("allow_empty_section")) {
            if (obj["allow_empty_section"].isBool()) {
                allow_empty_section = obj["allow_empty_section"].toBool();
            } else {
                throw DataEngineError(DataEngineErrorNumber::allow_empty_section_with_wrong_type,
                                      QString("\"Allow_empty_section\" -tag of section \"%1\" must be boolean. But is not.").arg(get_section_name()));

            }
        }
        if (obj.contains("variants")) {
            QJsonValue var_val = obj["variants"];
            if (var_val.isArray()) {
                for (auto variant : var_val.toArray()) {
                    append_variant_from_json(variant.toObject());
                }
            } else if (var_val.isObject()) {
                append_variant_from_json(var_val.toObject());
            } else {
                throw std::runtime_error(QString("invalid variant in json file").toStdString());
            }
        } else {
            append_variant_from_json(object.toObject());
        }
    } else {
        throw std::runtime_error(QString("invalid variant in json file").toStdString());
    }
}

QString DataEngineSection::get_section_name() const {
    return section_name;
}

QString DataEngineSection::get_instance_count_name() const {
    return instance_count_name;
}

bool DataEngineSection::is_section_instance_defined() const {
    return (bool)instance_count;
}

bool DataEngineSection::set_instance_count_if_name_matches(QString instance_count_name, uint instance_count) {
    bool result = false;

    if (this->instance_count_name == instance_count_name) {
        if (is_section_instance_defined()) {
            throw DataEngineError(
                DataEngineErrorNumber::instance_count_already_defined,
                QString("Instance count of section \"%1\" is already defined. Instance count name is: \"%2\", actual value of instance count is: %3.")
                    .arg(get_section_name())
                    .arg(instance_count_name)
                    .arg(this->instance_count.value()));
        } else {
            if (instance_count == 0) {
                throw DataEngineError(
                    DataEngineErrorNumber::instance_count_must_not_be_zero_nor_fraction_nor_negative,
                    QString("Instance count of section \"%1\" must not be zero. in section: \"%2\"").arg(get_section_name()).arg(section_name));
            }
            result = true;

            this->instance_count = instance_count;
            create_instances_if_defined();
        }
    }
    return result;
}

void DataEngineSection::create_instances_if_defined() {
    if (is_section_instance_defined()) {
        if (instance_count.value() == 0) {
            throw DataEngineError(DataEngineErrorNumber::instance_count_must_not_be_zero_nor_fraction_nor_negative,
                                  QString("Instance count of section \"%1\" must not be zero.").arg(get_section_name()));
        }
        assert(instance_captions.count() == 0);
        for (uint i = 0; i < instance_count.value(); i++) {
            instance_captions.append("");
        }
        VariantData *variant = const_cast<VariantData *>(get_variant());
        assert(variant);
        variant->set_instance_count(instance_count.value());
    }
}

bool DataEngineSection::is_allow_empty_section() const
{
    return allow_empty_section;
}

void DataEngineSection::use_instance(QString instance_caption, uint instance_index) {
    if (is_section_instance_defined() == false) {
        throw DataEngineError(
            DataEngineErrorNumber::instance_count_yet_undefined,
            QString("Instance count of section \"%1\" yet undefined. Instance count value is: \"%2\".").arg(get_section_name()).arg(get_instance_count_name()));
    }
    instance_index--;
    if (instance_index >= instance_count.value()) {
        throw DataEngineError(DataEngineErrorNumber::instance_count_exceeding, QString("Instance index (%1) of section \"%2\" exceeds instance count(%3).")
                                                                                   .arg(instance_index)
                                                                                   .arg(get_section_name())
                                                                                   .arg(instance_count.value()));
    }
    assert(instance_captions.count() > instance_index);
    instance_captions[instance_index] = instance_caption;
    VariantData *variant = const_cast<VariantData *>(get_variant());
    variant->set_actual_instance_index(instance_index);
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

void VariantData::set_instance_count(uint instance_count) {
    for (auto &data_entry : data_entries) {
        data_entry.get()->set_instance_count(instance_count);
    }
}

void VariantData::set_actual_instance_index(uint instance_index) {
    for (auto &data_entry : data_entries) {
        data_entry.get()->set_actual_instance_index(instance_index);
    }
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

bool Data_engine::is_complete() const {
    return sections.is_complete();
}

bool Data_engine::all_values_in_range() const {
    return sections.all_values_in_range();
}

bool Data_engine::value_in_range(const FormID &id) const {
    const DataEngineDataEntry *data_entry = sections.get_entry(id);
    assert(data_entry);
    return data_entry->is_in_range();
}

void Data_engine::set_actual_number(const FormID &id, double number) {
    DataEngineDataEntry *data_entry = sections.get_entry(id);
    assert(data_entry); //if field is not found an exception was already thrown above. Something bad must happen to assert here

    NumericDataEntry *number_entry = data_entry->as<NumericDataEntry>();
    ReferenceDataEntry *reference_entry = data_entry->as<ReferenceDataEntry>();
    if (!number_entry) {
        if (!reference_entry) {
            throw DataEngineError(DataEngineErrorNumber::setting_desired_value_with_wrong_type,
                                  QString("The field \"%1\" is not numerical as it must be if you set it with the number (%2) ").arg(id).arg(number));
        }
        reference_entry->set_actual_value(number);
    } else {
        number_entry->set_actual_value(number);
    }
}

void Data_engine::set_actual_text(const FormID &id, QString text) {
    DataEngineDataEntry *data_entry = sections.get_entry(id);
    assert(data_entry); //if field is not found an exception was already thrown above. Something bad must happen to assert here

    auto text_entry = data_entry->as<TextDataEntry>();
    ReferenceDataEntry *reference_entry = data_entry->as<ReferenceDataEntry>();
    if (!text_entry) {
        if (!reference_entry) {
            throw DataEngineError(DataEngineErrorNumber::setting_desired_value_with_wrong_type,
                                  QString("The field \"%1\" is not a string as it must be if you set it with the string \"%2\"").arg(id).arg(text));
        }
        reference_entry->set_actual_value(text);
    } else {
        text_entry->set_actual_value(text);
    }
}

void Data_engine::set_actual_bool(const FormID &id, bool value) {
    DataEngineDataEntry *data_entry = sections.get_entry(id);
    assert(data_entry); //if field is not found an exception was already thrown above. Something bad must happen to assert here

    auto bool_entry = data_entry->as<BoolDataEntry>();
    ReferenceDataEntry *reference_entry = data_entry->as<ReferenceDataEntry>();
    if (!bool_entry) {
        if (!reference_entry) {
            throw DataEngineError(DataEngineErrorNumber::setting_desired_value_with_wrong_type,
                                  QString("The field \"%1\" is not boolean as it must be if you set it with a bool type").arg(id));
        }
        reference_entry->set_actual_value(value);
    } else {
        bool_entry->set_actual_value(value);
    }
}

void Data_engine::use_instance(const QString &section_name, const QString &instance_caption, const uint instance_index) {
    sections.use_instance(section_name, instance_caption, instance_index);
}

void Data_engine::set_instance_count(QString instance_count_name, uint instance_count) {
    sections.set_instance_count(instance_count_name, instance_count);
}

QStringList Data_engine::get_actual_values(const FormID &id) const {
    const DataEngineDataEntry *data_entry = sections.get_entry(id);
    assert(data_entry);
    return data_entry->get_actual_values();
}

QString Data_engine::get_description(const FormID &id) const {
    const DataEngineDataEntry *data_entry = sections.get_entry(id);
    assert(data_entry);
    return data_entry->get_description();
}

//double Data_engine::get_desired_value(const FormID &id) const {
//    const DataEngineDataEntry *data_entry = sections.get_entry(id);
//    assert(data_entry);
//    return data_entry->as<NumericDataEntry>()->desired_value.value_or(std::numeric_limits<double>::quiet_NaN());
//}

QString Data_engine::get_desired_value_as_string(const FormID &id) const {
    const DataEngineDataEntry *data_entry = sections.get_entry(id);
    assert(data_entry);
    return data_entry->get_desired_value_as_string();
}

QString Data_engine::get_unit(const FormID &id) const {
    const DataEngineDataEntry *data_entry = sections.get_entry(id);
    assert(data_entry);
    return data_entry->get_unit();
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

    if (is_undefined) {
        throw DataEngineError(DataEngineErrorNumber::tolerance_must_be_defined_for_range_checks_on_numbers,
                              "no tolerance defined even though a range check should be done.");
    }
    if ((bool)measured == false) {
        return false;
    }

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

bool NumericTolerance::is_defined() const {
    return !is_undefined;
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
            std::experimental::optional<double> si_prefix = 1;
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
            if ((tolerance.is_defined() == false) && (entry_without_value == false)) {
                throw DataEngineError(DataEngineErrorNumber::tolerance_must_be_defined_for_numbers,
                                      "The number field with key \"" + field_name +
                                          "\" has no tolerance defined. Each number field must have a tolerance defined.");
            }
            return std::make_unique<NumericDataEntry>(field_name, desired_value, tolerance, std::move(unit), si_prefix, std::move(nice_name));
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
            return std::make_unique<BoolDataEntry>(field_name, desired_value, nice_name);
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
            return std::make_unique<TextDataEntry>(field_name, desired_value, nice_name);
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
            return std::make_unique<ReferenceDataEntry>(field_name, reference_string, tolerance, nice_name);
        }
        case EntryType::Unspecified: {
            throw DataEngineError(DataEngineErrorNumber::invalid_data_entry_type, "Invalid type in JSON object");
        }
    }
    throw DataEngineError(DataEngineErrorNumber::invalid_json_object, "invalid JSON object");
}

NumericDataEntry::NumericDataEntry(FormID field_name, std::experimental::optional<double> desired_value, NumericTolerance tolerance, QString unit,
                                   std::experimental::optional<double> si_prefix, QString description)
    : DataEngineDataEntry(field_name)
    , desired_value(desired_value)
    , tolerance(tolerance)
    , unit(std::move(unit))
    , description(std::move(description)) {
    this->si_prefix = si_prefix.value_or(1.0);
}

bool NumericDataEntry::is_complete() const {
    for (const auto &actual_v : actual_values) {
        if ((bool)actual_v == false) {
            return false;
        }
    }
    return actual_values.size() > 0;
}

bool NumericDataEntry::is_in_range() const {
    if (is_complete() == false) {
        return false;
    }
    if ((bool)desired_value) {
        for (const auto &actual_v : actual_values) {
            if (tolerance.test_in_range(desired_value.value(), actual_v) == false) {
                return false;
            }
        }
    }
    return true;
}

QString NumericDataEntry::get_desired_value_as_string() const {
    if ((bool)desired_value) {
        return tolerance.to_string(desired_value.value());
    } else {
        return "";
    }
}

QStringList NumericDataEntry::get_actual_values() const {
    QStringList result;
    for (const auto &actual_v : actual_values) {
        if ((bool)actual_v) {
            result.append(QString::number(actual_v.value()));
        } else {
            result.append(unavailable_value);
        }
    }
    return result;
}

QString NumericDataEntry::get_description() const {
    return description;
}

QString NumericDataEntry::get_unit() const {
    return unit;
}

void NumericDataEntry::set_actual_value(double actual_value) {
    assert(actual_instance_index < actual_values.size());
    actual_values[actual_instance_index] = actual_value / si_prefix;
}

void NumericDataEntry::set_instance_count(uint instance_count) {
    assert(actual_values.size() == 0);
    for (uint i = 0; i < instance_count; i++) {
        std::experimental::optional<double> empty_value{};
        actual_values.push_back(empty_value);
    }
}

uint NumericDataEntry::get_instance_count() {
    return actual_values.size();
}

void NumericDataEntry::set_actual_instance_index(uint instance_index) {
    assert(instance_index < actual_values.size());
    actual_instance_index = instance_index;
}

void NumericDataEntry::set_desired_value_from_desired(DataEngineDataEntry *from) {
    NumericDataEntry *num_from = from->as<NumericDataEntry>();
    assert(num_from);
    desired_value = num_from->desired_value;
}

void NumericDataEntry::set_desired_value_from_actual(DataEngineDataEntry *from) {
    NumericDataEntry *num_from = from->as<NumericDataEntry>();
    assert(num_from);
    assert(num_from->actual_values.size() == 1);
    desired_value = num_from->actual_values[0];
}

bool NumericDataEntry::is_desired_value_set() {
    return (bool)desired_value;
}

TextDataEntry::TextDataEntry(const FormID name, std::experimental::optional<QString> desired_value, QString description)
    : DataEngineDataEntry(name)
    , desired_value(std::move(desired_value))
    , description(description) {}

bool TextDataEntry::is_complete() const {
    for (const auto &actual_v : actual_values) {
        if ((bool)actual_v == false) {
            return false;
        }
    }
    return actual_values.size() > 0;
}

bool TextDataEntry::is_in_range() const {
    if (is_complete() == false) {
        return false;
    }
    if ((bool)desired_value) {
        for (const auto &actual_v : actual_values) {
            if (actual_v.value() != desired_value) {
                return false;
            }
        }
    }
    return true;
}

QStringList TextDataEntry::get_actual_values() const {
    QStringList result;
    for (const auto &actual_v : actual_values) {
        if ((bool)actual_v) {
            result.append(actual_v.value());
        } else {
            result.append(unavailable_value);
        }
    }
    return result;
}

QString TextDataEntry::get_description() const {
    return description;
}

QString TextDataEntry::get_desired_value_as_string() const {
    if ((bool)desired_value) {
        return desired_value.value();
    } else {
        return "";
    }
}

QString TextDataEntry::get_unit() const {
    return "";
}

void TextDataEntry::set_actual_value(QString actual_value) {
    assert(actual_instance_index < actual_values.size());
    this->actual_values[actual_instance_index] = actual_value;
}

void TextDataEntry::set_instance_count(uint instance_count) {
    assert(actual_values.size() == 0);
    for (uint i = 0; i < instance_count; i++) {
        std::experimental::optional<QString> empty_value{};
        actual_values.push_back(empty_value);
    }
}

uint TextDataEntry::get_instance_count() {
    return actual_values.size();
}

void TextDataEntry::set_actual_instance_index(uint instance_index) {
    assert(instance_index < actual_values.size());
    actual_instance_index = instance_index;
}

void TextDataEntry::set_desired_value_from_desired(DataEngineDataEntry *from) {
    TextDataEntry *num_from = from->as<TextDataEntry>();
    assert(num_from);
    desired_value = num_from->desired_value;
}

void TextDataEntry::set_desired_value_from_actual(DataEngineDataEntry *from) {
    TextDataEntry *num_from = from->as<TextDataEntry>();
    assert(num_from);
    assert(num_from->actual_values.size() == 1);
    desired_value = num_from->actual_values[0];
}

bool TextDataEntry::is_desired_value_set() {
    return (bool)desired_value;
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

BoolDataEntry::BoolDataEntry(const FormID name, std::experimental::optional<bool> desired_value, QString description)
    : DataEngineDataEntry(name)
    , desired_value(std::move(desired_value))
    , description(description) {}

bool BoolDataEntry::is_complete() const {
    for (const auto &actual_v : actual_values) {
        if ((bool)actual_v == false) {
            return false;
        }
    }
    return actual_values.size() > 0;
}

bool BoolDataEntry::is_in_range() const {
    if (is_complete() == false) {
        return false;
    }
    if ((bool)desired_value) {
        for (const auto &actual_v : actual_values) {
            if (actual_v.value() != desired_value) {
                return false;
            }
        }
    }
    return true;
}

QStringList BoolDataEntry::get_actual_values() const {
    QStringList result;
    for (const auto &actual_v : actual_values) {
        if ((bool)actual_v == false) {
            result.append(unavailable_value);
        } else if (actual_v.value()) {
            result.append("true");
        } else {
            result.append("false");
        }
    }
    return result;
}

QString BoolDataEntry::get_description() const {
    return description;
}

QString BoolDataEntry::get_desired_value_as_string() const {
    if ((bool)desired_value) {
        if (desired_value.value()) {
            return "true";
        } else {
            return "false";
        }
    } else {
        return "";
    }
}

QString BoolDataEntry::get_unit() const {
    return "";
}

void BoolDataEntry::set_actual_value(bool value) {
    assert(actual_instance_index < actual_values.size());
    actual_values[actual_instance_index] = value;
}

void BoolDataEntry::set_instance_count(uint instance_count) {
    assert(actual_values.size() == 0);
    for (uint i = 0; i < instance_count; i++) {
        std::experimental::optional<bool> empty_value{};
        actual_values.push_back(empty_value);
    }
}

uint BoolDataEntry::get_instance_count() {
    return actual_values.size();
}

void BoolDataEntry::set_actual_instance_index(uint instance_index) {
    assert(instance_index < actual_values.size());
    actual_instance_index = instance_index;
}

void BoolDataEntry::set_desired_value_from_desired(DataEngineDataEntry *from) {
    BoolDataEntry *num_from = from->as<BoolDataEntry>();
    assert(num_from);
    desired_value = num_from->desired_value;
}

void BoolDataEntry::set_desired_value_from_actual(DataEngineDataEntry *from) {
    BoolDataEntry *bool_from = from->as<BoolDataEntry>();
    assert(bool_from);
    assert(bool_from->actual_values.size() == 1);
    desired_value = bool_from->actual_values[0];
}

bool BoolDataEntry::is_desired_value_set() {
    return (bool)desired_value;
}

ReferenceDataEntry::ReferenceDataEntry(const FormID name, QString reference_string, NumericTolerance tolerance, QString description)
    : DataEngineDataEntry(name)
    , tolerance(tolerance)
    , description(description) {
    parse_refence_string(reference_string);
}

bool ReferenceDataEntry::is_complete() const {
    update_desired_value_from_reference();
    if (!entry->is_desired_value_set()) {
        return false;
    }
    return entry->is_complete();
}

bool ReferenceDataEntry::is_in_range() const {
    update_desired_value_from_reference();
    if (!entry->is_desired_value_set()) {
        return false;
    }
    return entry->is_in_range();
}

void ReferenceDataEntry::update_desired_value_from_reference() const {
    if (reference_links[0].value == ReferenceLink::ReferenceValue::DesiredValue) {
        assert(entry_target->is_desired_value_set());
        entry->set_desired_value_from_desired(entry_target);
    } else {
        if (entry_target->get_instance_count() != 1) {
            throw DataEngineError(DataEngineErrorNumber::reference_must_not_point_to_multiinstance_actual_value,
                                  QString("References must not point to the actual value of multi-instance-fields. The referencing fieldname: \"%1\", the "
                                          "reference target is: \"%2\"")
                                      .arg(field_name)
                                      .arg(entry_target->field_name));
        }
        entry->set_desired_value_from_actual(entry_target);
    }
}

void ReferenceDataEntry::set_actual_value(double number) {
    NumericDataEntry *num_entry = entry->as<NumericDataEntry>();
    if (!num_entry) {
        throw DataEngineError(
            DataEngineErrorNumber::setting_reference_actual_value_with_wrong_type,
            QString("The referencing field \"%1\" is not a number as it must be if you set it with the number: \"%2\"").arg(field_name).arg(number));
    }

    num_entry->set_actual_value(number);
}

void ReferenceDataEntry::set_actual_value(QString val) {
    TextDataEntry *text_entry = entry->as<TextDataEntry>();
    if (!text_entry) {
        throw DataEngineError(
            DataEngineErrorNumber::setting_reference_actual_value_with_wrong_type,
            QString("The referencing field \"%1\" is not a string as it must be if you set it with the string: \"%2\"").arg(field_name).arg(val));
    }
    text_entry->set_actual_value(val);
}

void ReferenceDataEntry::set_actual_value(bool val) {
    BoolDataEntry *bool_entry = entry->as<BoolDataEntry>();
    if (!bool_entry) {
        throw DataEngineError(DataEngineErrorNumber::setting_reference_actual_value_with_wrong_type,
                              QString("The referencing field \"%1\" is not a bool as it must be if you set it with the bool: \"%2\"").arg(field_name).arg(val));
    }
    bool_entry->set_actual_value(val);
}

void ReferenceDataEntry::set_instance_count(uint instance_count) {
    entry->set_instance_count(instance_count);
}

uint ReferenceDataEntry::get_instance_count() {
    return entry->get_instance_count();
}

void ReferenceDataEntry::set_actual_instance_index(uint instance_index) {
    entry->set_actual_instance_index(instance_index);
}

QStringList ReferenceDataEntry::get_actual_values() const {
    return entry->get_actual_values();
}

QString ReferenceDataEntry::get_description() const {
    return description;
}

QString ReferenceDataEntry::get_desired_value_as_string() const {
    update_desired_value_from_reference();
    return entry->get_desired_value_as_string();
}

QString ReferenceDataEntry::get_unit() const {
    return entry_target->get_unit();
}

void ReferenceDataEntry::dereference(DataEngineSections *sections) {
    uint i = 0;
    while (i < reference_links.size()) {
        if (sections->exists_uniquely(reference_links[i].link)) {
            i++;
        } else {
            reference_links.erase(reference_links.begin() + i);
        }
    }
    if (reference_links.size() == 0) {
        throw DataEngineError(DataEngineErrorNumber::reference_not_found, QString("reference non existing. Fieldname: \"%1\"").arg(field_name));

    } else if (reference_links.size() > 1) {
        QString t;
        for (auto &ref : reference_links) {
            t += ref.link + " ";
        }
        throw DataEngineError(DataEngineErrorNumber::reference_ambiguous,
                              QString("reference ambiguous  with links: \"%1\", fieldname: \"%2\"").arg(t).arg(field_name));
    }
    entry_target = sections->get_entry(reference_links[0].link);

    if (reference_links[0].value == ReferenceLink::ReferenceValue::DesiredValue) {
        if (!entry_target->is_desired_value_set()) {
            throw DataEngineError(DataEngineErrorNumber::reference_target_has_no_desired_value,
                                  QString("reference \"%1\" points to desired value \"%2\", even though no desired value is defined in \"%2\".")
                                      .arg(field_name)
                                      .arg(reference_links[0].link));
        }
    }

    NumericDataEntry *num_entry = entry_target->as<NumericDataEntry>();
    TextDataEntry *text_entry = entry_target->as<TextDataEntry>();
    BoolDataEntry *bool_entry = entry_target->as<BoolDataEntry>();
    if (num_entry) {
        std::experimental::optional<double> temp_desired_value; //will be set later, when beeing compared
        if (!tolerance.is_defined()) {
            throw DataEngineError(
                DataEngineErrorNumber::reference_is_a_number_and_needs_tolerance,
                QString("reference \"%1\" pointing to \"%2\", is a number but has no tolerance defined. A tolerance must be defined on numbers.")
                    .arg(field_name)
                    .arg(reference_links[0].link));
        }
        entry = std::make_unique<NumericDataEntry>("", temp_desired_value, tolerance, num_entry->unit, num_entry->si_prefix, description);
    } else if (text_entry) {
        std::experimental::optional<QString> temp_desired_value; //will be set later, when beeing compared
        entry = std::make_unique<TextDataEntry>("", temp_desired_value, description);
    } else if (bool_entry) {
        std::experimental::optional<bool> temp_desired_value; //will be set later, when beeing compared
        entry = std::make_unique<BoolDataEntry>("", temp_desired_value, description);
    } else {
        assert(0); //TODO:throw illegal type
    }
    if (!num_entry) {
        if (tolerance.is_defined()) {
            throw DataEngineError(
                DataEngineErrorNumber::reference_is_not_number_but_has_tolerance,
                QString(
                    "reference \"%1\" pointing to \"%2\", is not a number but has a tolerance defined. A tolerance is only allowed to be applied on numbers.")
                    .arg(field_name)
                    .arg(reference_links[0].link));
        }
    }
}

void ReferenceDataEntry::parse_refence_string(QString reference_string) {
    const QString ACTUAL_VALUE = ".actual";
    const QString DESIRED_VALUE = ".desired";
    reference_links.clear();
    if (reference_string.startsWith("[")) {
        reference_string.remove(0, 1);
    }
    if (reference_string.endsWith("]")) {
        reference_string.remove(reference_string.size() - 1, 1);
    }
    auto refs = reference_string.split(",");
    for (auto &ref : refs) {
        ref = ref.trimmed();
        ReferenceLink ref_link;
        if (ref.endsWith(ACTUAL_VALUE)) {
            ref = ref.remove(ref.size() - ACTUAL_VALUE.size(), ACTUAL_VALUE.size());
            ref_link.value = ReferenceLink::ReferenceValue::ActualValue;
            ref_link.link = ref;
        } else if (ref.endsWith(DESIRED_VALUE)) {
            ref = ref.remove(ref.size() - DESIRED_VALUE.size(), DESIRED_VALUE.size());
            ref_link.value = ReferenceLink::ReferenceValue::DesiredValue;
            ref_link.link = ref;
        } else {
            throw DataEngineError(DataEngineErrorNumber::illegal_reference_declaration,
                                  QString("reference \"%1\" target declaration must end with \".desired\" or \".actual\" but does not.").arg(field_name));
        }
        reference_links.push_back(ref_link);
    }
}

void ReferenceDataEntry::set_desired_value_from_desired(DataEngineDataEntry *from) {
    (void)from;
    assert(0);
}

void ReferenceDataEntry::set_desired_value_from_actual(DataEngineDataEntry *from) {
    (void)from;
    assert(0);
}

bool ReferenceDataEntry::is_desired_value_set() {
    assert(0);
}
