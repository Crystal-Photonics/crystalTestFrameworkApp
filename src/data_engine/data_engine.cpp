
#include "data_engine.h"
//#if IAM_NOT_LUPDATE
#include "data_engine_strings.h"
//#endif
#include "Windows/mainwindow.h"
#include "exceptionalapproval.h"
#include "lua_functions.h"
#include "util.h"
#include "vc.h"
#include <QApplication>
#include <QBuffer>
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QObject>
#include <QPainter>
#include <QPrinter>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlRecord>
#include <QStringList>
#include <QTemporaryFile>
#include <QUrl>
#include <QWidget>
#include <QXmlStreamWriter>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <lrdatasourcemanagerintf.h>
#include <lrreportengine.h>
#include <type_traits>

static const QString exceptional_approvals_table_name = "exceptional_approvals";
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

bool have_entries_equal_desired_values(QList<const DataEngineDataEntry *> entries) {
    if (entries.count() == 0) {
        return false;
    }
    const auto first_entry = entries[0];
    for (auto entry : entries) {
        if (first_entry->compare_unit_desired_siprefix(entry) == false) {
            return false;
        }
    }
    return true;
}

Data_engine::Data_engine(std::istream &source, const QMap<QString, QList<QVariant>> &tags)
    : sections{} {
    set_dependancy_tags(tags);
    set_source(source);
}

Data_engine::Data_engine(std::istream &source)
    : sections{} {
    set_source(source);
}

void Data_engine::set_dependancy_tags(const QMap<QString, QList<QVariant>> &tags) {
    sections.set_dependancy_tags(tags);
    sections.is_dummy_data_mode = false;
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
        throw DataEngineError(DataEngineErrorNumber::invalid_json_file,
                              "Dataengine: Invalid JSON file.\n\nYou can use one of the online JSON validators you will find at google.");
    }

    sections.from_json(document.object());
    sections.create_already_defined_instances();

    sections.delete_unmatched_variants();

    sections.deref_references();
    load_time_seconds_since_epoch = QDateTime::currentMSecsSinceEpoch() / 1000;
}

void Data_engine::set_script_path(QString script_path) {
    this->script_path = script_path;
}

void Data_engine::set_source_path(QString source_path) {
    this->source_path = source_path;
}

void Data_engine::start_recording_actual_value_statistic(const std::string &root_file_path, const std::string &file_prefix) {
    statistics_file.start_recording(QString::fromStdString(root_file_path), QString::fromStdString(file_prefix));
}

void Data_engine::set_dut_identifier(QString dut_identifier)
{
    statistics_file.set_dut_identifier(dut_identifier);
}

void Data_engine::save_actual_value_statistic()
{
    statistics_file.save_to_file();
}

QStringList Data_engine::get_instance_count_names() {
    QStringList result;
    for (auto &section : sections.sections) {
        auto name = section.get_instance_count_name();
        if ((name.count()) && !result.contains(name)) {
            result.append(name);
        }
    }
    return result;
}

void Data_engine::set_start_time_seconds_since_epoch(double start_seconds_since_epoch) {
    this->load_time_seconds_since_epoch = static_cast<qint64>(start_seconds_since_epoch);
}

DataEngineSections::DataEngineSections() {}

void DataEngineSections::delete_unmatched_variants() {
    if (is_dummy_data_mode) {
        for (auto &section : sections) {
            section.delete_all_but_biggest_variants();
        }
    } else {
        for (auto &section : sections) {
            section.delete_unmatched_variants(dependency_tags);
        }
    }
}

void DataEngineSections::deref_references() {
    for (auto &section : sections) {
        for (auto &instance : section.instances) {
            if (instance.variants.size() == 0) {
                continue;
            }
            const VariantData *variant_to_test = instance.get_variant();
            for (const auto &entry : variant_to_test->data_entries) {
                ReferenceDataEntry *reference = entry.get()->as<ReferenceDataEntry>();
                if (reference) {
                    reference->dereference(this);
                }
            }
        }
    }
}

void DataEngineSections::set_instance_count(QString instance_count_name, uint instance_count) {
    bool instance_count_name_found = false;
    for (auto &section : sections) {
        if (section.set_instance_count_if_name_matches(instance_count_name, instance_count)) {
            if (is_dummy_data_mode) {
                section.delete_all_but_biggest_variants();
            } else {
                section.delete_unmatched_variants(dependency_tags);
            }
            instance_count_name_found = true;
        }
    }
    deref_references();
    if (instance_count_name_found == false) {
        throw DataEngineError(DataEngineErrorNumber::instance_count_does_not_exist,
                              QString("Dataengine: Could not find instance count name:\"%1\"").arg(instance_count_name));
    }
}

void DataEngineSections::use_instance(QString section_name, QString instance_caption, uint instance_index) {
    DataEngineErrorNumber error_num = DataEngineErrorNumber::ok;
    DataEngineSection *section_to_use = get_section_raw(section_name, &error_num);

    if (error_num == DataEngineErrorNumber::no_section_id_found) {
        throw DataEngineError(DataEngineErrorNumber::no_section_id_found, QString("Dataengine: Could not find section with name = \"%1\"").arg(section_name));
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

DataEngineSection *DataEngineSections::get_section(const FormID &id) const {
    DataEngineErrorNumber error_num = DataEngineErrorNumber::ok;
    DecodecFieldID field_id = decode_field_id(id);
    DataEngineSection *result_section = get_section_raw(field_id.section_name, &error_num);
    if (result_section == nullptr)
        throw DataEngineError(DataEngineErrorNumber::no_section_id_found, "Dataengine: Section not found");
    return result_section;
}

DataEngineSection *DataEngineSections::get_section_no_exception(FormID id) const {
    DataEngineErrorNumber error_num = DataEngineErrorNumber::ok;
    if (!id.contains("/")) {
        id += "/dummy";
    }
    DecodecFieldID field_id = decode_field_id(id);
    DataEngineSection *result_section = get_section_raw(field_id.section_name, &error_num);
    return result_section;
}

DecodecFieldID DataEngineSections::decode_field_id(const FormID &id) {
    DecodecFieldID result;

    auto names = id.split("/");
    if (names.count() != 2) {
        throw DataEngineError(DataEngineErrorNumber::faulty_field_id,
                              QString("Dataengine: field id needs to be in format \"section-name/field-name\" but is %1").arg(id));
    }

    result.section_name = names[0];
    result.field_name = names[1];
    return result;
}

void DataEngineSections::set_dependancy_tags(const QMap<QString, QList<QVariant>> &tags) {
    dependency_tags = tags;
}

const QMap<QString, QList<QVariant>> &DataEngineSections::get_dependancy_tags() const {
    return dependency_tags;
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

QList<const DataEngineDataEntry *> DataEngineSections::get_entries_raw(const FormID &id, DataEngineErrorNumber *error_num, DecodecFieldID &decoded_field_name,
                                                                       bool using_instance_index) const {
    *error_num = DataEngineErrorNumber::ok;
    decoded_field_name = decode_field_id(id);
    DataEngineSection *section = get_section_raw(decoded_field_name.section_name, error_num);

    QList<const DataEngineDataEntry *> result;

    if (section) {
        if (section->is_section_instance_defined() == false) {
            assert(0); //exception should already have been thrown elsewere
            //throw DataEngineError(DataEngineErrorNumber::instance_count_yet_undefined,
            //                      QString("Dataengine: Instance count of section \"%1\" yet undefined. Instance count value is: \"%2\".")
            //                          .arg(section->get_section_name())
            //                          .arg(section->get_instance_count_name()));
        }

        for (uint index = 0; index < section->instances.size(); index++) {
            auto &instance = section->instances[index];
            if (using_instance_index && (index != section->get_actual_instance_index())) {
                continue;
            }

            const VariantData *variant = instance.get_variant();
            assert(variant);
            DataEngineErrorNumber error_num_dummy;
            const DataEngineDataEntry *item = variant->get_entry_raw(decoded_field_name.field_name, &error_num_dummy);
            if (item == nullptr) {
            } else {
                result.append(item);
            }
        }

        if (result.count() == 0) {
            *error_num = DataEngineErrorNumber::no_field_id_found;
        }
        return result;
    }

    return {};
}

QList<const DataEngineDataEntry *> DataEngineSections::get_entries_const(const FormID &id) const {
    DataEngineErrorNumber error_num = DataEngineErrorNumber::ok;
    DecodecFieldID decoded_field_id;
    auto result = get_entries_raw(id, &error_num, decoded_field_id, false);
    if (error_num == DataEngineErrorNumber::no_field_id_found) {
        throw DataEngineError(DataEngineErrorNumber::no_field_id_found,
                              QString("Dataengine: Could not find field with name = \"%1\"").arg(decoded_field_id.field_name));
    } else if (error_num == DataEngineErrorNumber::no_section_id_found) {
        throw DataEngineError(DataEngineErrorNumber::no_section_id_found,
                              QString("Dataengine: Could not find section with name = \"%1\"").arg(decoded_field_id.section_name));
    } else if (error_num != DataEngineErrorNumber::ok) {
        assert(0);
    }

    return result;
}

const DataEngineDataEntry *DataEngineSections::get_actual_instance_entry_const(const FormID &id) const {
    DataEngineErrorNumber error_num = DataEngineErrorNumber::ok;
    DecodecFieldID decoded_field_id;
    auto result_list = get_entries_raw(id, &error_num, decoded_field_id, true);
    if (error_num == DataEngineErrorNumber::no_field_id_found) {
        throw DataEngineError(DataEngineErrorNumber::no_field_id_found,
                              QString("Dataengine: Could not find field with name = \"%1\"").arg(decoded_field_id.field_name));
    } else if (error_num == DataEngineErrorNumber::no_section_id_found) {
        throw DataEngineError(DataEngineErrorNumber::no_section_id_found,
                              QString("Dataengine: Could not find section with name = \"%1\"").arg(decoded_field_id.section_name));
    } else if (error_num != DataEngineErrorNumber::ok) {
        assert(0);
    }
    assert(result_list.size() == 1);

    return result_list[0];
}

QList<DataEngineDataEntry *> DataEngineSections::get_entries(const FormID &id) {
    QList<DataEngineDataEntry *> result;
    auto const_results = get_entries_const(id);
    for (auto const_result : const_results) {
        result.append(const_cast<DataEngineDataEntry *>(const_result));
    }

    return result;
}

bool DataEngineSections::exists_uniquely(const FormID &id) const {
    DataEngineErrorNumber error_num = DataEngineErrorNumber::ok;

    bool result = true;
    DecodecFieldID decoded_field_id;
    auto entries = get_entries_raw(id, &error_num, decoded_field_id, false);
    if (error_num == DataEngineErrorNumber::no_field_id_found) {
        result = false;
    } else if (error_num == DataEngineErrorNumber::no_section_id_found) {
        result = false;
    } else if (error_num != DataEngineErrorNumber::ok) {
        assert(0);
        result = false;
    }
    result = result && (entries.count() > 0);

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
            throw std::runtime_error(QString("Dataengine: invalid json object %1").arg(key).toStdString());
        }
        DataEngineSection section;
        section.from_json(section_object, key);
        if (section_exists(section.get_section_name())) {
            throw DataEngineError(DataEngineErrorNumber::duplicate_section, QString("Dataengine: duplicate section %1").arg(section.get_section_name()));
        }
        sections.push_back(std::move(section));
    }
}

bool DataEngineSections::section_exists(QString section_name) {
    DataEngineErrorNumber error_num;
    get_section_raw(section_name, &error_num);
    return error_num == DataEngineErrorNumber::ok;
}

void DataEngineSection::delete_unmatched_variants(const QMap<QString, QList<QVariant>> &tags) {
    int instance_index = 0;
    for (DataEngineInstance &instance : instances) {
        assert((bool)instance_count);
        instance.delete_unmatched_variants(tags, instance_index, instance_count.value());
        instance_index++;
    }
}

void DataEngineSection::delete_all_but_biggest_variants() {
    int instance_index = 0;
    for (auto &instance : instances) {
        assert((bool)instance_count);
        instance.delete_all_but_biggest_variants();
        instance_index++;
    }
}

DataEngineInstance::DataEngineInstance() {}

DataEngineInstance::DataEngineInstance(const DataEngineInstance &other)
    : variants{other.variants}
    , instance_caption{other.instance_caption}
    , section_name{other.section_name}
    , allow_empty_section{other.allow_empty_section} {}

DataEngineInstance::DataEngineInstance(DataEngineInstance &&other)
    : variants(std::move(other.variants))
    , instance_caption{std::move(other.instance_caption)}
    , section_name{std::move(other.section_name)}
    , allow_empty_section{other.allow_empty_section} {}

const VariantData *DataEngineInstance::get_variant() const {
    if (variants.size() == 1) {
        return &variants[0];
    }

    throw DataEngineError(DataEngineErrorNumber::no_variant_found,
                          QString("Dataengine: No dependency fullfilling variant found in section: \"%1\"").arg(section_name));
    return nullptr;
}

void DataEngineInstance::delete_unmatched_variants(const QMap<QString, QList<QVariant>> &tags, uint instance_index, uint instance_count) {
    std::vector<VariantData> variants_new;

    for (auto &variant : variants) {
        if (variant.is_dependency_matching(tags, instance_index, instance_count, section_name)) {
            variants_new.push_back(std::move(variant));
            //i++;
        }
    }
    variants = std::move(variants_new);
    if (variants.size() > 1) {
        throw DataEngineError(
            DataEngineErrorNumber::non_unique_desired_field_found,
            QString("Dataengine: More than one dependency fullfilling variants (%1) found in section: \"%2\"").arg(variants.size()).arg(section_name));
    }

    if ((allow_empty_section) && (variants.size() == 0)) {
        VariantData variant_data{};
        variants.push_back(std::move(variant_data));
    }
    get_variant(); //for throwing exception if no variant was found
}

void DataEngineInstance::delete_all_but_biggest_variants() {
    std::vector<VariantData> variants_new;
    std::experimental::optional<uint> max_size;
    for (auto &variant : variants) {
        if (max_size < variant.get_entry_count()) {
            max_size = variant.get_entry_count();
        }
    }

    for (auto &variant : variants) {
        if (max_size == variant.get_entry_count()) {
            variants_new.push_back(std::move(variant));
            break;
        }
    }

    variants = std::move(variants_new);
    if (variants.size() > 1) {
        throw DataEngineError(
            DataEngineErrorNumber::non_unique_desired_field_found,
            QString("Dataengine: More than one dependency fullfilling variants (%1) found in section: \"%2\"").arg(variants.size()).arg(section_name));
    }

    if ((allow_empty_section) && (variants.size() == 0)) {
        VariantData variant_data{};
        variants.push_back(std::move(variant_data));
    }
    get_variant(); //for throwing exception if no variant was found
}

void DataEngineInstance::set_section_name(QString section_name) {
    this->section_name = section_name;
}

void DataEngineInstance::set_allow_empty_section(bool allow_empty_section) {
    this->allow_empty_section = allow_empty_section;
}

bool DataEngineSection::is_complete() const {
    bool result = true;
    if (is_section_instance_defined() == false) {
        result = false;
    }
    for (auto &instance : instances) {
        const VariantData *variant_to_test = instance.get_variant();
        assert(variant_to_test);
        for (const auto &entry : variant_to_test->data_entries) {
            if (!entry.get()->is_complete()) {
                result = false;
            }
        }
    }
    return result;
}

bool DataEngineSection::all_values_in_range() const {
    bool result = is_complete();
    for (auto &instance : instances) {
        const VariantData *variant_to_test = instance.get_variant();
        for (const auto &entry : variant_to_test->data_entries) {
            if (!entry.get()->is_in_range()) {
                result = false;
            }
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
    return "Unknown";
}

void DataEngineSection::from_json(const QJsonValue &object, const QString &key_name) {
    section_name = key_name;
    prototype_instance.set_section_name(section_name);
    if (object.isArray()) {
        for (auto variant : object.toArray()) {
            const QJsonObject &obj = variant.toObject();
            if (obj.contains("instance_count")) {
                throw DataEngineError(
                    DataEngineErrorNumber::instance_count_must_not_be_defined_in_variant_scope,
                    QString("Dataengine: Instance count of section \"%1\" must not be a defined within variant scope.").arg(get_section_name()));
            }
            if (obj.contains("allow_empty_section")) {
                throw DataEngineError(
                    DataEngineErrorNumber::allow_empty_section_must_not_be_defined_in_variant_scope,
                    QString("Dataengine: allow_empty_section of section \"%1\" must not be a defined within variant scope.").arg(get_section_name()));
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
                                      QString("Dataengine: Instance count of section \"%1\" must not be either integer or string. But is \"%2\".")
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
                                          QString("Dataengine: Instance count of section \"%1\" must not be a fraction.").arg(get_section_name()));
                }

                if (instance_count_dbl_json < 0) {
                    throw DataEngineError(DataEngineErrorNumber::instance_count_must_not_be_zero_nor_fraction_nor_negative,
                                          QString("Dataengine: Instance count of section \"%1\" must not be negative.").arg(get_section_name()));
                }
            }

        } else {
            instance_count = 1;
            instance_count_name = "";
        }
        if (obj.contains("allow_empty_section")) {
            if (obj["allow_empty_section"].isBool()) {
                prototype_instance.set_allow_empty_section(obj["allow_empty_section"].toBool());
            } else {
                throw DataEngineError(
                    DataEngineErrorNumber::allow_empty_section_with_wrong_type,
                    QString("Dataengine: \"Allow_empty_section\" -tag of section \"%1\" must be boolean. But is not.").arg(get_section_name()));
            }
        }
        if (obj.contains("title")) {
            section_title = obj["title"].toString();
        } else {
            section_title = section_name;
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
                throw std::runtime_error(QString("Dataengine: invalid variant in json file").toStdString());
            }
        } else {
            append_variant_from_json(object.toObject());
        }
    } else {
        throw std::runtime_error(QString("Dataengine: invalid variant in json file").toStdString());
    }
}

QString DataEngineSection::get_section_name() const {
    return section_name;
}

QString DataEngineSection::get_instance_count_name() const {
    return instance_count_name;
}

static QString adjust_sql_table_name(QString old_table_name) {
    old_table_name = old_table_name.toLower();
    old_table_name = old_table_name.replace("-", "_");
    old_table_name = old_table_name.replace("ü", "ue");
    old_table_name = old_table_name.replace("ö", "oe");
    old_table_name = old_table_name.replace("ä", "ae");
    old_table_name = old_table_name.replace("ß", "sz");
    return old_table_name;
}

QString DataEngineSection::get_sql_section_name() const {
    return adjust_sql_table_name(get_section_name());
}

QString DataEngineSection::get_sql_instance_name() const {
    return get_sql_section_name() + "_instance";
}

QString DataEngineSection::get_actual_instance_caption() const {
    return get_instance_captions()[actual_instance_index];
}

QStringList DataEngineSection::get_all_ids_of_selected_instance(const QString &prefix) const {
    QStringList result;
    auto instance = get_actual_instance();
    assert(instance);
    auto variant = instance->get_variant();
    assert(variant);
    for (const auto &entry : variant->data_entries) {
        result.append(prefix + entry.get()->field_name);
    }
    return result;
}

QStringList DataEngineSection::get_instance_captions() const {
    QStringList result;
    assert_instance_is_defined();
    for (const auto &instance : instances) {
        result.append(instance.instance_caption);
    }
    return result;
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
                QString(
                    "Dataengine: Instance count of section \"%1\" is already defined. Instance count name is: \"%2\", actual value of instance count is: %3.")
                    .arg(get_section_name())
                    .arg(instance_count_name)
                    .arg(this->instance_count.value()));
        } else {
            if (instance_count == 0) {
                throw DataEngineError(
                    DataEngineErrorNumber::instance_count_must_not_be_zero_nor_fraction_nor_negative,
                    QString("Dataengine: Instance count of section \"%1\" must not be zero. in section: \"%2\"").arg(get_section_name()).arg(section_name));
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
                                  QString("Dataengine: Instance count of section \"%1\" must not be zero.").arg(get_section_name()));
        }
        assert(instances.size() == 0);
        for (uint i = 0; i < instance_count.value(); i++) {
            //
            DataEngineInstance instance(prototype_instance);
            instances.push_back(instance);
        }
    }
}

void DataEngineSection::assert_instance_is_defined() const {
    if (is_section_instance_defined() == false) {
        throw DataEngineError(DataEngineErrorNumber::instance_count_yet_undefined,
                              QString("Dataengine: Instance count of section \"%1\" yet undefined. Instance count value is: \"%2\".")
                                  .arg(get_section_name())
                                  .arg(get_instance_count_name()));
    }
}

void DataEngineSection::use_instance(QString instance_caption, uint instance_index) {
    assert_instance_is_defined();
    instance_index--;
    if (instance_index >= instance_count.value()) {
        throw DataEngineError(DataEngineErrorNumber::instance_count_exceeding,
                              QString("Dataengine: Instance index (%1) of section \"%2\" exceeds instance count(%3).")
                                  .arg(instance_index)
                                  .arg(get_section_name())
                                  .arg(instance_count.value()));
    }
    assert(instances.size() > instance_index);
    instances[instance_index].instance_caption = instance_caption;
    actual_instance_index = instance_index;
}

void DataEngineSection::append_variant_from_json(const QJsonObject &object) {
    VariantData variant;
    variant.from_json(object);
    prototype_instance.variants.push_back(std::move(variant));
}

VariantData::VariantData() {
    //
}

VariantData::VariantData(const VariantData &other)
    : dependency_tags{other.dependency_tags}
    , relevant_dependencies{other.relevant_dependencies} {
    for (auto &entry : other.data_entries) {
        auto entry_num = dynamic_cast<NumericDataEntry *>(entry.get());
        auto entry_bool = dynamic_cast<BoolDataEntry *>(entry.get());
        auto entry_text = dynamic_cast<TextDataEntry *>(entry.get());
        auto entry_ref = dynamic_cast<ReferenceDataEntry *>(entry.get());
        if (entry_num) {
            data_entries.push_back(std::make_unique<NumericDataEntry>(*entry_num));
        } else if (entry_bool) {
            data_entries.push_back(std::make_unique<BoolDataEntry>(*entry_bool));
        } else if (entry_text) {
            data_entries.push_back(std::make_unique<TextDataEntry>(*entry_text));
        } else if (entry_ref) {
            data_entries.push_back(std::make_unique<ReferenceDataEntry>(*entry_ref));
        } else {
            assert(0);
            //unknown data type
        }
    }
}

uint VariantData::get_entry_count() const {
    return data_entries.size();
}

QString VariantData::get_dependencies_serialised_string() const {
    return dependency_tags.get_dependencies_serialised_string();
}

bool VariantData::uses_dependency() const {
    return !dependency_tags.tags.isEmpty();
}

bool VariantData::is_dependency_matching(const QMap<QString, QList<QVariant>> &tags, uint instance_index, uint instance_count, const QString &section_name) {
    relevant_dependencies.clear();
    bool dependency_matches = true;
    const auto &keys_to_test = dependency_tags.tags.uniqueKeys();
    std::experimental::optional<uint> length_of_first_tag_value_list;
    if (keys_to_test.count()) {
        for (const auto &tag_key : keys_to_test) {
            if (tags.contains(tag_key)) {
                const unsigned int value_count = tags[tag_key].count();
                if ((bool)length_of_first_tag_value_list == false) {
                    length_of_first_tag_value_list = value_count;
                } else {
                    if (value_count != length_of_first_tag_value_list.value()) {
                        throw DataEngineError(DataEngineErrorNumber::list_of_dependency_values_must_be_of_equal_length,
                                              QString("Dataengine: In section \"%1\": The length of the tables of corresponding dependency tag values must be "
                                                      "equal. But is %2 instead of %3.")
                                                  .arg(section_name)
                                                  .arg(value_count)
                                                  .arg(length_of_first_tag_value_list.value()));
                    }
                }
            }
        }
        if ((bool)length_of_first_tag_value_list == false) {
            return false; // if there were no tags matching, it is quite obvious that the result is false
        }

        if ((instance_count != length_of_first_tag_value_list.value()) && (length_of_first_tag_value_list.value() != 1)) {
            throw DataEngineError(
                DataEngineErrorNumber::instance_count_must_match_list_of_dependency_values,
                QString("Dataengine: Instance count of section \"%1\" must have the same value as the length of the tables of corresponding dependency "
                        "tag values (%2). But is %3")
                    .arg(section_name)
                    .arg(length_of_first_tag_value_list.value())
                    .arg(instance_count));
        }

        if (length_of_first_tag_value_list.value() == 1) {
            instance_index = 0;
        }

        assert(instance_index < instance_count);
    }

    for (const auto &tag_key : keys_to_test) {
        if (tags.contains(tag_key)) {
            bool at_least_value_matches = false;
            const auto &value_list = dependency_tags.tags.values(tag_key);
            for (const auto &test_matching : value_list) {
                auto val = tags[tag_key][instance_index];
                if (test_matching.is_matching(val)) {
                    at_least_value_matches = true;
                    relevant_dependencies.insert(tag_key, val);
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
    if (!dependency_matches) {
        relevant_dependencies.clear();
    }
    return dependency_matches;
}

void VariantData::from_json(const QJsonObject &object) {
    auto depend_tags = object["apply_if"];
    QJsonValue data = object["data"];

    if (data.isUndefined() || data.isNull()) {
        throw DataEngineError(DataEngineErrorNumber::no_data_section_found, QString("Dataengine: no data found ininvalid variant in json file"));
    }

    if (data.isArray()) {
        if (data.toArray().count() == 0) {
            throw DataEngineError(DataEngineErrorNumber::no_data_section_found, QString("Dataengine: no data found in variant in json file"));
        }
        for (const auto &data_entry : data.toArray()) {
            std::unique_ptr<DataEngineDataEntry> entry = DataEngineDataEntry::from_json(data_entry.toObject());
            if (entry_exists(entry.get()->field_name)) {
                throw DataEngineError(DataEngineErrorNumber::duplicate_field,
                                      QString("Dataengine: Data field with the name %1 is already existing").arg(entry.get()->field_name));
            }
            data_entries.push_back(std::move(entry));
        }
    } else if (data.isObject()) {
        std::unique_ptr<DataEngineDataEntry> entry = DataEngineDataEntry::from_json(data.toObject());
        if (entry_exists(entry.get()->field_name)) {
            throw DataEngineError(DataEngineErrorNumber::duplicate_field,
                                  QString("Dataengine: Data field with the name %1 is already existing").arg(entry.get()->field_name));
        }
        data_entries.push_back(std::move(entry));
    }
    dependency_tags.from_json(depend_tags);
}

bool VariantData::entry_exists(QString field_name) {
    DataEngineErrorNumber errornum;
    return get_entry_raw(field_name, &errornum) != nullptr;
}

DataEngineDataEntry *VariantData::get_entry(QString field_name) const {
    DataEngineErrorNumber errornum;
    auto result = get_entry_raw(field_name, &errornum);
    if (errornum == DataEngineErrorNumber::no_field_id_found) {
        throw DataEngineError(DataEngineErrorNumber::no_field_id_found, QString("Dataengine: Could not find field with name = \"%1\"").arg(field_name));
    } else if (errornum == DataEngineErrorNumber::ok) {
    } else {
        assert(0);
        throw DataEngineError(errornum, QString("Dataengine: UnkownError at \"%1\"").arg(field_name));
    }

    return result;
}

DataEngineDataEntry *VariantData::get_entry_raw(QString field_name, DataEngineErrorNumber *errornum) const {
    *errornum = DataEngineErrorNumber::ok;
    for (const auto &entry : data_entries) {
        if (entry.get()->field_name == field_name) {
            return entry.get();
        }
    }
    *errornum = DataEngineErrorNumber::no_field_id_found;
    return nullptr;
}

const QMap<QString, QVariant> &VariantData::get_relevant_dependencies() const {
    return relevant_dependencies;
}

QString DependencyTags::get_dependencies_serialised_string() const {
    QString result = "";
    for (const QString &key : tags.keys()) {
        const auto &val = tags.value(key);
        result = result + key + ":" + val.get_serialised_string();
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
                throw std::runtime_error(QString("Dataengine: invalid type of tag description in json file").toStdString());
            }
        }
    } else if (object.isUndefined()) {
        // nothing to do
    } else {
        throw std::runtime_error(QString("Dataengine: invalid tag description in json file").toStdString());
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

QString DependencyValue::get_serialised_string() const {
    QString str = serialised_string;
    switch (match_style) {
        case Match_style::MatchEverything:
            str = "all_" + str;
            break;
        case Match_style::MatchExactly:
            str = "exact_" + str;
            break;
        case Match_style::MatchByRange:
            str = "range_" + str;
            break;

        case Match_style::MatchNone:
            str = "none_" + str;
            break;
    }
    return str;
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
        throw std::runtime_error(QString("Dataengine: invalid version dependency in json file").toStdString());
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
    return false;
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
                                  QString("Dataengine: could not parse dependency string \"%1\" in as range number").arg(str));
        }
    }
}

void DependencyValue::from_string(const QString &str) {
    QString value = str.trimmed();
    serialised_string = value;
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
                                  QString("Dataengine: invalid version dependency string \"%1\"in json file").arg(str));
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
    serialised_string = QString::number(number);
    range_low_including = 0;
    range_high_excluding = 0;
}

void DependencyValue::from_bool(const bool &boolean) {
    match_style = Match_style::MatchExactly;
    match_exactly.setValue<bool>(boolean);
    if (boolean) {
        serialised_string = "true";
    } else {
        serialised_string = "false";
    }

    range_low_including = 0;
    range_high_excluding = 0;
}

bool Data_engine::is_complete() const {
    assert_in_dummy_mode();
    return sections.is_complete();
}

bool Data_engine::all_values_in_range() const {
    assert_in_dummy_mode();
    return sections.all_values_in_range();
}

bool Data_engine::values_in_range(const QList<FormID> &ids) const {
    for (FormID const &id : ids) {
        if (!value_in_range(id)) {
            return false;
        }
    }
    return true;
}

bool Data_engine::value_complete_in_instance(const FormID &id) const {
    assert_in_dummy_mode();
    auto data_entry = sections.get_actual_instance_entry_const(id);
    assert(data_entry);
    bool result = data_entry->is_complete();
    return result;
}

bool Data_engine::value_in_range_in_instance(const FormID &id) const {
    assert_in_dummy_mode();
    auto data_entry = sections.get_actual_instance_entry_const(id);
    assert(data_entry);
    bool result = data_entry->is_in_range();
    return result;
}

bool Data_engine::value_complete_in_section(FormID id) const {
    assert_in_dummy_mode();
    if (!id.contains("/")) {
        id = id + "/";
    }
    const DataEngineSection *section = sections.get_section(id);
    return section->is_complete();
}

bool Data_engine::value_in_range_in_section(FormID id) const {
    assert_in_dummy_mode();
    if (!id.contains("/")) {
        id = id + "/";
    }
    const DataEngineSection *section = sections.get_section(id);
    return section->all_values_in_range();
}

bool Data_engine::value_in_range(const FormID &id) const {
    assert_in_dummy_mode();
    QList<const DataEngineDataEntry *> data_entry = sections.get_entries_const(id);
    for (auto entry : data_entry) {
        assert(entry);
        if (!entry->is_in_range()) {
            return false;
        }
    }
    return true;
}

bool Data_engine::value_complete(const FormID &id) const {
    assert_in_dummy_mode();
    QList<const DataEngineDataEntry *> data_entry = sections.get_entries_const(id);
    for (auto entry : data_entry) {
        assert(entry);
        if (!entry->is_complete()) {
            return false;
        }
    }
    return true;
}

DataEngineDataEntry *DataEngineSection::get_entry(QString id) const {
    const auto instance = get_actual_instance();
    assert(instance);
    auto variant = instance->get_variant();
    assert(variant);
    auto field_id = DataEngineSections::decode_field_id(id);
    auto data_entry = variant->get_entry(field_id.field_name);
    return data_entry;
}

const DataEngineInstance *DataEngineSection::get_actual_instance() const {
    if (is_section_instance_defined() == false) {
        throw DataEngineError(DataEngineErrorNumber::instance_count_yet_undefined,
                              QString("Dataengine: Instance count of section \"%1\" yet undefined. Instance count value is: \"%2\".")
                                  .arg(get_section_name())
                                  .arg(get_instance_count_name()));
    }
    assert(actual_instance_index < instances.size());
    return &instances[actual_instance_index];
}

QString DataEngineSection::get_serialised_dependency_string() const {
    const auto instance = get_actual_instance();
    assert(instance);
    auto variant = instance->get_variant();
    return variant->get_dependencies_serialised_string();
}

void DataEngineSection::set_actual_number(const FormID &id, double number) {
    auto data_entry = get_entry(id);
    assert(data_entry); //if field is not found an exception was already thrown above. Something bad must happen to assert here

    NumericDataEntry *number_entry = data_entry->as<NumericDataEntry>();
    ReferenceDataEntry *reference_entry = data_entry->as<ReferenceDataEntry>();
    if (!number_entry) {
        if (!reference_entry) {
            throw DataEngineError(
                DataEngineErrorNumber::setting_desired_value_with_wrong_type,
                QString("Dataengine: The field \"%1\" is not numerical as it must be if you set it with the number (%2) ").arg(id).arg(number));
        }
        reference_entry->set_actual_value(number);
    } else {
        number_entry->set_actual_value(number);
    }
}

void DataEngineSection::set_actual_text(const FormID &id, QString text) {
    auto *data_entry = get_entry(id);
    assert(data_entry); //if field is not found an exception was already thrown above. Something bad must happen to assert here

    auto text_entry = data_entry->as<TextDataEntry>();
    ReferenceDataEntry *reference_entry = data_entry->as<ReferenceDataEntry>();
    if (!text_entry) {
        if (!reference_entry) {
            throw DataEngineError(DataEngineErrorNumber::setting_desired_value_with_wrong_type,
                                  QString("Dataengine: The field \"%1\" is not a string as it must be if you set it with the string \"%2\"").arg(id).arg(text));
        }
        reference_entry->set_actual_value(text);
    } else {
        text_entry->set_actual_value(text);
    }
}

void DataEngineSection::set_actual_bool(const FormID &id, bool value) {
    DataEngineDataEntry *data_entry = get_entry(id);
    assert(data_entry); //if field is not found an exception was already thrown above. Something bad must happen to assert here

    auto bool_entry = data_entry->as<BoolDataEntry>();
    ReferenceDataEntry *reference_entry = data_entry->as<ReferenceDataEntry>();
    if (!bool_entry) {
        if (!reference_entry) {
            throw DataEngineError(DataEngineErrorNumber::setting_desired_value_with_wrong_type,
                                  QString("Dataengine: The field \"%1\" is not boolean as it must be if you set it with a bool type").arg(id));
        }
        reference_entry->set_actual_value(value);
    } else {
        bool_entry->set_actual_value(value);
    }
}

uint DataEngineSection::get_actual_instance_index() {
    return actual_instance_index;
}

QString DataEngineSection::get_section_title() const {
    return section_title;
}

bool DataEngineSection::section_uses_variants() const {
    if (prototype_instance.variants.size() > 1)
        return true;
    const VariantData *variant = prototype_instance.get_variant();
    assert(variant);
    return variant->uses_dependency();
}

void Data_engine::set_actual_number(const FormID &id, double number) {
    auto section = sections.get_section(id);
    section->set_actual_number(id, number);
    QString serialised_dependency = section->get_serialised_dependency_string();
    statistics_file.set_actual_value(id, serialised_dependency, number);
}

void Data_engine::set_actual_text(const FormID &id, QString text) {
    auto section = sections.get_section(id);
    section->set_actual_text(id, text);
}

void Data_engine::set_actual_bool(const FormID &id, bool value) {
    auto section = sections.get_section(id);
    section->set_actual_bool(id, value);
}

void Data_engine::use_instance(const QString &section_name, const QString &instance_caption, const uint instance_index) {
    sections.use_instance(section_name, instance_caption, instance_index);
}

void Data_engine::set_instance_count(QString instance_count_name, uint instance_count) {
    sections.set_instance_count(instance_count_name, instance_count);
}

QStringList Data_engine::get_instance_captions(const QString &section_name) const {
    DataEngineSection *the_section = sections.get_section(section_name + "/dummy");
    assert(the_section);
    return the_section->get_instance_captions();
}

bool Data_engine::section_uses_variants(QString section_name) const {
    DataEngineSection *the_section = sections.get_section(section_name + "/dummy");
    assert(the_section);
    return the_section->section_uses_variants();
}

bool Data_engine::section_uses_instances(QString section_name) const {
    DataEngineSection *the_section = sections.get_section(section_name + "/dummy");
    assert(the_section);
    return the_section->get_instance_count_name().size() || the_section->get_instance_captions().count() > 1;
}

void Data_engine::set_enable_auto_open_pdf(bool auto_open_pdf) {
    this->auto_open_pdf = auto_open_pdf;
}

QStringList Data_engine::get_ids_of_section(const QString &section_name) const {
    DataEngineSection *the_section = sections.get_section(section_name + "/dummy");
    assert(the_section);
    return the_section->get_all_ids_of_selected_instance(section_name + "/");
}

void Data_engine::fill_engine_with_dummy_data() {
    assert(sections.is_dummy_data_mode);
    auto section_names = get_section_names();
    for (const auto &sectionname : section_names) {
        for (uint instance_i = 1; instance_i <= get_instance_count(sectionname.toStdString()); instance_i++) {
            use_instance(sectionname, "Instance-caption #" + QString::number(instance_i), instance_i);
            auto ids = get_ids_of_section(sectionname);
            for (const auto &id : ids) {
                if (is_bool(id)) {
                    set_actual_bool(id, true);
                } else if (is_number(id)) {
                    set_actual_number(id, 1);
                } else if (is_text(id)) {
                    set_actual_text(id, "test 123");
                } else {
                    assert(0);
                }
            }
        }
    }
}

QStringList Data_engine::get_section_names() const {
    QStringList result;
    for (auto &section : sections.sections) {
        result.append(section.get_section_name());
    }
    return result;
}

sol::table Data_engine::get_ids_of_section(sol::state *lua, const std::string &section_name) {
    sol::table result = lua->create_table_with();
    auto id_list = get_ids_of_section(QString::fromStdString(section_name));
    for (const auto &id : id_list) {
        result.add(id.toStdString());
    }
    return result;
}

sol::table Data_engine::get_section_names(sol::state *lua) {
    sol::table result = lua->create_table_with();
    auto section_names = get_section_names();
    for (const auto &section_name : section_names) {
        result.add(section_name.toStdString());
    }
    return result;
}

uint Data_engine::get_instance_count(const std::string &section_name) const {
    return get_instance_captions(QString::fromStdString(section_name)).size();
}

QString Data_engine::get_actual_value(const FormID &id) const {
    assert_in_dummy_mode();
    auto data_entry = sections.get_actual_instance_entry_const(id);
    QString result;
    assert(data_entry);
    result = data_entry->get_actual_values();
    return result;
}

double Data_engine::get_actual_number(const FormID &id) const {
    assert_in_dummy_mode();
    auto data_entry = sections.get_actual_instance_entry_const(id);
    assert(data_entry);
    auto result = data_entry->get_actual_number();
    return result;
}

QString Data_engine::get_description(const FormID &id) const {
    assert_in_dummy_mode();
    auto data_entry = sections.get_actual_instance_entry_const(id);
    QString result;
    assert(data_entry);
    result = data_entry->get_description();
    return result;
}

bool Data_engine::is_desired_value_set(const FormID &id) const {
    assert_in_dummy_mode();
    auto data_entry = sections.get_actual_instance_entry_const(id);
    assert(data_entry);
    return data_entry->is_desired_value_set();
}

bool Data_engine::is_bool(const FormID &id) const {
    auto data_entry = sections.get_actual_instance_entry_const(id);
    assert(data_entry);
    return data_entry->get_entry_type() == EntryType::Bool;
}

bool Data_engine::is_number(const FormID &id) const {
    auto data_entry = sections.get_actual_instance_entry_const(id);
    assert(data_entry);
    return data_entry->get_entry_type() == EntryType::Numeric;
}

bool Data_engine::is_text(const FormID &id) const {
    auto data_entry = sections.get_actual_instance_entry_const(id);
    assert(data_entry);
    return data_entry->get_entry_type() == EntryType::String;
}

bool Data_engine::is_exceptionally_approved(const FormID &id) const {
    auto data_entry = sections.get_actual_instance_entry_const(id);
    assert(data_entry);
    return data_entry->get_exceptional_approval().approved;
}

QString Data_engine::get_desired_value_as_string(const FormID &id) const {
    assert_in_dummy_mode();
    auto data_entry = sections.get_actual_instance_entry_const(id);
    QString result;
    assert(data_entry);
    result = data_entry->get_desired_value_as_string();
    return result;
}

QString Data_engine::get_unit(const FormID &id) const {
    assert_in_dummy_mode();
    auto data_entry = sections.get_actual_instance_entry_const(id);
    QString result;
    assert(data_entry);
    result = data_entry->get_unit();
    return result;
}

EntryType Data_engine::get_entry_type(const FormID &id) const {
    assert_in_dummy_mode();
    auto data_entry = sections.get_actual_instance_entry_const(id);
    assert(data_entry);
    return data_entry->get_entry_type();
}

double Data_engine::get_si_prefix(const FormID &id) const {
    assert_in_dummy_mode();
    auto data_entry = sections.get_actual_instance_entry_const(id);
    assert(data_entry);
    return data_entry->get_si_prefix();
}

QString Data_engine::get_section_title(const QString section_name) const {
    auto section = sections.get_section(section_name + "/dummy");
    return section->get_section_title();
}

std::unique_ptr<QWidget> Data_engine::get_preview() const {
    LimeReport::ReportEngine re;
    re.previewReport();
    return nullptr;
}

bool Data_engine::generate_pdf(const std::string &form, const std::string &destination) const {
    QString db_name = ""; //TODO: find a better temporary name
                          //QDir::homePath()
                          //      AppLocalDataLocation
#if 1
    QTemporaryFile db_file;
    if (db_file.open()) {
        db_name = db_file.fileName(); // returns the unique file name
                                      //The file name of the temporary file can be found by calling fileName().
                                      //Note that this is only defined after the file is first opened; the
                                      //function returns an empty string before this.
    }
    db_file.close(); //Reopening a QTemporaryFile after calling close() is safe
                     // QFile::remove(db_name);
                     // assert(QFile{db_name}.exists() == false);
#endif
    QSqlDatabase db;
    if (QSqlDatabase::contains()) {
        db = QSqlDatabase::database();
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }

    db.setDatabaseName(db_name);
    bool opend = db.open();
    if (!opend) {
        qDebug() << "SQLConnection error: " << db.lastError().text();
    }
    assert(opend); //TODO

    if (!QFile{QString::fromStdString(form)}.exists()) {
        qDebug() << "PDF Template file does not exist: " << QString::fromStdString(form);
        throw DataEngineError(DataEngineErrorNumber::pdf_template_file_not_existing,
                              "Dataengine: PDF Template file does not exist: " + QString::fromStdString(form));
    }
    fill_database(db);
    const auto sourced_form = form + ".tmp.lrxml";
    replace_database_filename(form, sourced_form, QFileInfo{db_name}.absoluteFilePath().toStdString());

    LimeReport::ReportEngine re;
    if (re.loadFromFile(QString::fromStdString(sourced_form), false) == false) {
        return false;
    }
    bool result = false;
    {
        QString filename = QString::fromStdString(destination);
        //doing the error checking that LimeReport should do but doesn't.
        QPrinter printer;
        printer.setOutputFileName(filename);
        QPainter painter;
        if (painter.begin(&printer) == false) {
            MainWindow::mw->execute_in_gui_thread([filename] {
                QMessageBox::critical(MainWindow::mw, "Failed printing report",
                                      "Requested to write report to file " + filename + " which could not be opened.");
            });
        } else {
            painter.end();
            result = re.printToPDF(filename);
        }
    }
    db.close();
    if (auto_open_pdf) {
        QFileInfo fi{QString::fromStdString(destination)};
        auto p = fi.absoluteFilePath();
        if (!p.startsWith("/")) {
            p = "/" + p;
        }
        QDesktopServices::openUrl(QUrl("file://" + p));
    }
    return result;
}

static void db_exec(QSqlDatabase &db, QString query) {
    //    qDebug() << query;
    auto instance = db.exec(query);
    if (instance.lastError().isValid()) {
        qDebug() << instance.lastError().text();
        throw DataEngineError(
            DataEngineErrorNumber::sql_error,
            QString{"Dataengine: Failed executing query: ''%1''. Error: %2 \n\nProbably because database file ''%3'' is opened by another program."}.arg(
                query, instance.lastError().text(), db.databaseName()));
    }
}

static QString get_caption(const QString &section_name, const QString &instance_caption) {
    (void)section_name;
    if (instance_caption.isEmpty()) {
        return "";
    }
    return instance_caption;
}

void Data_engine::save_to_json(QString filename) {
    QFile saveFile(filename);

    if (filename == "") {
        throw DataEngineError(DataEngineErrorNumber::cannot_open_file, QString{"Dataengine: Filename for dumping data_engine is empty"});
        return;
    }

    if (!saveFile.open(QIODevice::WriteOnly)) {
        throw DataEngineError(DataEngineErrorNumber::cannot_open_file,
                              QString{"Dataengine: Can not open file: \"%1\" for dumping data_engine content."}.arg(filename));
        return;
    }

    bool exceptional_approval_exists = false;
    QJsonObject jo_general;

    auto script_git = git_info(QFileInfo(script_path).absolutePath(), true, false);
    jo_general["datetime_unix"] = QDateTime::currentMSecsSinceEpoch() / 1000;
    jo_general["datetime_str"] = QDateTime::currentDateTime().toString("yyyy:MM:dd HH:mm:ss");

    jo_general["framework_git_hash"] = "0x" + QString::number(GITHASH, 16);
    jo_general["script_path"] = script_path;
    jo_general["data_source_path"] = source_path;

    if (script_git.contains("hash")) {
        jo_general["test_git_hash"] = script_git["hash"].toString();
    }
    if (script_git.contains("date")) {
        jo_general["test_git_date_str"] = script_git["date"].toString();
    }
    if (script_git.contains("modified")) {
        jo_general["test_git_modified"] = script_git["modified"].toString();
    }

    jo_general["everything_in_range"] = all_values_in_range();
    jo_general["everything_complete"] = is_complete();

    QVariant duration{};
    duration.setValue<qint64>(QDateTime::currentMSecsSinceEpoch() / 1000 - load_time_seconds_since_epoch);
    jo_general["test_duration_seconds"] = QJsonValue::fromVariant(duration);
    jo_general["os_username"] = QString::fromStdString(get_os_username());

    QJsonObject jo_dependency;
    const QMap<QString, QList<QVariant>> dependency_tags = sections.get_dependancy_tags();
    for (auto k : dependency_tags.keys()) {
        QJsonArray ja;
        auto values = dependency_tags.values(k);
        for (auto v : values) {
            ja.append(QJsonValue::fromVariant(v));
        }
        jo_dependency[k] = ja;
    }

    QJsonObject ja_sections;

    for (const DataEngineSection &section : sections.sections) {
        QJsonObject jo_section;
        QJsonArray ja_instances;

        const auto section_name = section.get_section_name();

        for (const DataEngineInstance &instance : section.instances) {
            QJsonObject jo_instance;
            QJsonArray ja_fields;
            const auto variant = instance.get_variant();
            assert(variant);
            auto relevant_dependencies = variant->get_relevant_dependencies();
            auto jo_relevant_dependencies = QJsonObject::fromVariantMap(relevant_dependencies);

            for (const std::unique_ptr<DataEngineDataEntry> &entry : variant->data_entries) {
                QJsonObject jo_field;
                jo_field["name"] = entry->field_name;
                jo_field["nice_name"] = entry->get_description();
                jo_field["in_range"] = entry->is_in_range();
                jo_field["is_complete"] = entry->is_complete();
                if (entry->get_exceptional_approval().approved) {
                    jo_field["exceptional_approval"] = entry->get_exceptional_approval().get_json_dump();
                    exceptional_approval_exists = true;
                }
                jo_field[entry->get_specific_json_name()] = entry->get_specific_json_dump();
                ja_fields.append(jo_field);
            }

            jo_instance["instance_caption"] = instance.instance_caption;
            jo_instance["dependency_tags"] = jo_relevant_dependencies;
            jo_instance["fields"] = ja_fields;

            ja_instances.append(jo_instance);
        }
        jo_section["instances"] = ja_instances;
        ja_sections[section_name] = jo_section;
    }
    jo_general["exceptional_approval_exists"] = exceptional_approval_exists;
    QJsonObject ja_object;
    ja_object["general"] = jo_general;
    ja_object["dependency_tags"] = jo_dependency;
    ja_object["sections"] = ja_sections;

    QJsonDocument saveDoc(ja_object);
    saveFile.write(saveDoc.toJson());
}

void Data_engine::fill_database(QSqlDatabase &db) const {
    QList<ExceptionalApprovalResult> exceptional_approvals;

    for (const DataEngineSection &section : sections.sections) {
        int instance_id_counter{1};
        const auto instances_table_name = section.get_sql_instance_name();

        db_exec(db, QUERY_CREATE_INSTANCE_TABLE.arg(instances_table_name));

        const auto &section_table_name = section.get_sql_section_name();
        db_exec(db, QUERY_CREATE_DATA_TABLE.arg(section_table_name));
        int id = 0;
        for (const DataEngineInstance &instance : section.instances) {
            db_exec(db, QString{"INSERT INTO %1 VALUES(%2, '%3')"}.arg(instances_table_name, QString::number(instance_id_counter),
                                                                       get_caption(section_table_name, instance.instance_caption)));
            auto variant = instance.get_variant();
            assert(variant);

            for (const std::unique_ptr<DataEngineDataEntry> &entry : variant->data_entries) {
                QString failed_string = QObject::tr("Failed");
                const ExceptionalApprovalResult &ea = entry->get_exceptional_approval();
                if (ea.approved) {
                    failed_string += "(" + QString::number(ea.exceptional_approval.id) + ")";
                    if (!entry->is_in_range()) {
                        bool found = false;
                        for (const auto &ea_item : exceptional_approvals) { //grouping approvals
                            if (ea_item.exceptional_approval.id == ea.exceptional_approval.id) {
                                found = true;
                                break;
                            }
                        }
                        if (found == false) {
                            exceptional_approvals.append(ea);
                        }
                    }
                }
                db_exec(db, QString{"INSERT INTO %1 VALUES(%2, '%3', '%4', '%5', '%6', %7, '%8', '%9')"}.arg(
                                section_table_name, QString::number(id), section.get_section_name() + "/" + entry->field_name, entry->get_description(),
                                entry->get_desired_value_as_string(), entry->get_actual_values(), QString::number(instance_id_counter),
                                entry->is_in_range() ? QObject::tr("Ok") : failed_string, entry->get_unit()));
                id++;
            }

            instance_id_counter++;
        }
    }

    db_exec(db, QString{R"(
        CREATE TABLE %1 (
            ID int PRIMARY KEY,
            Description text,
            approving_person text
        )
    )"}.arg(exceptional_approvals_table_name));
    for (const auto &ea : exceptional_approvals) {
        db_exec(db, QString{"INSERT INTO %1 VALUES(%2, '%3', '%4')"}.arg(exceptional_approvals_table_name, QString::number(ea.exceptional_approval.id),
                                                                         ea.exceptional_approval.description, ea.approving_operator_name));
    }
}

void Data_engine::do_exceptional_approval_(ExceptionalApprovalDB &ea_db, QList<FailedField> failed_fields, QWidget *parent) {
    auto approvals = ea_db.select_exceptional_approval(failed_fields, parent);
    for (auto approval : approvals) {
        approval.failed_field.data_entry->set_exceptional_approval(approval);
    }
}

static FailedField failed_field_from_data_entry(const DataEngineDataEntry *entry, QString field_id, QString instance_caption) {
    FailedField failed_field{};
    failed_field.actual_value = entry->get_actual_values();
    failed_field.description = entry->get_description();
    failed_field.desired_value = entry->get_desired_value_as_string();
    failed_field.id = field_id;
    failed_field.instance_caption = instance_caption;
    failed_field.data_entry = const_cast<DataEngineDataEntry *>(entry);
    return failed_field;
}

bool Data_engine::do_exceptional_approval(ExceptionalApprovalDB &ea_db, QString field_id, QWidget *parent) {
    QList<FailedField> failed_fields;

    auto entry = sections.get_actual_instance_entry_const(field_id);
    if (((entry->is_in_range() == false) || (entry->is_complete() == false)) == true) {
        auto section = sections.get_section(field_id);
        assert(section);
        auto instance_caption = section->get_actual_instance_caption();
        FailedField failed_field = failed_field_from_data_entry(entry, field_id, instance_caption);
        failed_fields.append(failed_field);
    }

    auto approvals = ea_db.select_exceptional_approval(failed_fields, parent);
    for (auto approval : approvals) {
        approval.failed_field.data_entry->set_exceptional_approval(approval);
    }
    return approvals.count();
}

void Data_engine::do_exceptional_approvals(ExceptionalApprovalDB &ea_db, QWidget *parent) {
    QList<FailedField> failed_fields;
    int instance_id_counter = 1;
    for (const DataEngineSection &section : sections.sections) {
        for (const DataEngineInstance &instance : section.instances) {
            auto variant = instance.get_variant();
            assert(variant);

            for (const std::unique_ptr<DataEngineDataEntry> &entry : variant->data_entries) {
                if (((entry->is_in_range() == false) || (entry->is_complete() == false)) == true) {
                    FailedField failed_field =
                        failed_field_from_data_entry(entry.get(), section.get_section_name() + "/" + entry->field_name, instance.instance_caption);
                    failed_fields.append(failed_field);
                }
            }

            instance_id_counter++;
        }
    }
    do_exceptional_approval_(ea_db, failed_fields, parent);
}

struct XML_state {
    QXmlStreamWriter xml{};
    int y{};
    int band_index{};
};

struct XML {
    XML(const QString &name) {
        state.xml.writeStartElement(name);
    }
    XML &attribute(const QString &name, const QString &value) {
        state.xml.writeAttribute(name, value);
        return *this;
    }
    XML &attributes(const std::initializer_list<std::pair<QString, QString>> &list) {
        for (auto &item : list) {
            attribute(item.first, item.second);
        }
        return *this;
    }
    XML &value(const QString &value) {
        state.xml.writeCharacters(value);
        return *this;
    }
    ~XML() {
        state.xml.writeEndElement();
    }
    XML &add_band_index_element() {
        XML("bandIndex").attributes({{"Type", "int"}, {"Value", QString::number(state.band_index++)}});
        return *this;
    }

    static auto &y() {
        return state.y;
    }

    static XML_state state;
};

XML_state XML::state;

void Data_engine::generate_template(const QString &destination, const QString &db_filename, QString report_title, QString image_footer_path,
                                    QString image_header_path, QString approved_by_field_id, QString static_text_report_header, QString static_text_page_header,
                                    QString static_text_page_footer, QString static_text_report_footer_above_signature,
                                    QString static_text_report_footer_beneath_signature, const QList<PrintOrderItem> &print_order) const {
    QFile xml_file{destination};
    xml_file.open(QFile::OpenModeFlag::WriteOnly | QFile::OpenModeFlag::Truncate);
    XML::state.xml.setDevice(&xml_file);
    auto &xml = XML::state.xml;

    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("Report");
    {
        xml.writeStartElement("object");
        xml.writeAttribute("Type", "Object");
        xml.writeAttribute("ClassName", "LimeReport::ReportEnginePrivate");
        {
            xml.writeStartElement("pages");
            xml.writeAttribute("Type", "Collection");
            generate_pages(xml, report_title, image_footer_path, image_header_path, approved_by_field_id, static_text_report_header, static_text_page_header,
                           static_text_page_footer, static_text_report_footer_above_signature, static_text_report_footer_beneath_signature, print_order);
            xml.writeEndElement(); //pages

            xml.writeStartElement("datasourcesManager");
            xml.writeAttribute("Type", "Object");
            xml.writeAttribute("ClassName", "LimeReport::DataSourceManager");
            add_sources_to_form(db_filename, print_order, approved_by_field_id);
            xml.writeEndElement(); //datasourcesManager

            xml.writeStartElement("scriptContext");
            xml.writeAttribute("Type", "Object");
            xml.writeAttribute("ClassName", "LimeReport::ScriptEngineContext");
            xml.writeEndElement(); //scriptContext
        }
        xml.writeEndElement(); //"object"
    }
    xml.writeEndElement(); //Report
    xml.writeEndDocument();
}

void Data_engine::generate_pages(QXmlStreamWriter &xml, QString report_title, QString image_footer_path, QString image_header_path,
                                 QString approved_by_field_id, QString static_text_report_header, QString static_text_page_header,
                                 QString static_text_page_footer, QString static_text_report_footer_above_signature,
                                 QString static_text_report_footer_beneath_signature, const QList<PrintOrderItem> &print_order) const {
    XML pages{"item"};
    pages.attributes({{"Type", "Object"}, {"ClassName", "LimeReport::PageDesignIntf"}});
    {
        XML page_item{"pageItem"};
        page_item.attributes({{"Type", "Object"}, {"ClassName", "PageItem"}});
        XML{"objectName"}.attribute("Type", "QString").value("ReportPage");
        {
            XML children{"children"};
            children.attribute("Type", "Collection");
            generate_pages_header(xml, report_title, image_footer_path, image_header_path, approved_by_field_id, static_text_report_header,
                                  static_text_page_header, static_text_page_footer, static_text_report_footer_above_signature,
                                  static_text_report_footer_beneath_signature, print_order);
            generate_tables(print_order);
        }
    }
}

int Data_engine::generate_image(QXmlStreamWriter &xml, QString image_path, int y_position, QString parent_name) const {
    QString image_hex_string;
    const int desired_width = 2000;
    int aspect_ratio_height = 0;
    if (!QFile::exists(image_path)) {
        return y_position;
    }
    QImage image{image_path};
    bool image_valid = image.width() > 0;
    if (image_valid) {
        QByteArray ba;
        QBuffer buff(&ba);
        buff.open(QIODevice::WriteOnly);
        image.save(&buff, "PNG");
        image_hex_string = ba.toHex();
        aspect_ratio_height = image.height() * desired_width / image.width();

        XML image_item{"item"};
        image_item.attributes({{"Type", "Object"}, {"ClassName", "ImageItem"}});
        XML{"geometry"}.attributes({{"width", QString::number(desired_width)},
                                    {"height", QString::number(aspect_ratio_height)},
                                    {"x", "0"},
                                    {"y", QString::number(y_position)},
                                    {"Type", "QRect"}});

        xml.writeStartElement("parentName");
        {
            xml.writeAttribute("Type", "QString");
            xml.writeCharacters(parent_name);
        }
        xml.writeEndElement(); //objectName

        xml.writeStartElement("objectName");
        {
            xml.writeAttribute("Type", "QString");
            xml.writeCharacters("header_image");
        }
        xml.writeEndElement(); //objectName

        xml.writeStartElement("image");
        {
            xml.writeAttribute("Type", "QImage");
            xml.writeCharacters(image_hex_string);
        }
        xml.writeEndElement(); //objectName

        XML{"center"}.attributes({{"Type", "bool"}, {"Value", "1"}});
        XML{"scale"}.attributes({{"Type", "bool"}, {"Value", "1"}});
        XML{"keepAspectRatio"}.attributes({{"Type", "bool"}, {"Value", "1"}});
    }
    return aspect_ratio_height + y_position;
}

int Data_engine::generate_textfields(QXmlStreamWriter &xml, int y_start, const QList<PrintOrderItem> &print_order,
                                     TextFieldDataBandPlace actual_band_position) const {
    int y = y_start;
    int text_field_height = 50;
    int font_size = 10;
    QString font_color = "#000000";
    if ((actual_band_position == TextFieldDataBandPlace::page_footer) || (actual_band_position == TextFieldDataBandPlace::page_header)) {
        text_field_height = 30;
        font_size = 6;
        font_color = "#a8a8a8";
    }

    QList<PrintOrderSectionItem> textfield_print_order = get_print_order(print_order, true, actual_band_position);
    for (auto section_order : textfield_print_order) {
        assert(section_order.section->instances.size() == 1);
        auto variant = section_order.section->instances[0].get_variant();
        assert(variant);
        const int text_field_width = 2000 / section_order.print_order_item.text_field_column_count;
        unsigned int col = 0;
        for (const auto &fields : variant->data_entries) {
            const int x_position = col * text_field_width;
            DataEngineDataEntry *field = fields.get();
            xml.writeStartElement("item");
            {
                xml.writeAttribute("Type", "Object");
                xml.writeAttribute("ClassName", "TextItem");
                XML{"geometry"}.attributes({{"width", QString::number(text_field_width)},
                                            {"height", QString::number(text_field_height)},
                                            {"x", QString::number(x_position)},
                                            {"y", QString::number(y)},
                                            {"Type", "QRect"}});
                XML{"font"}.attributes({{"pointSize", QString::number(font_size)},
                                        {"family", "Arial"},
                                        {"weight", "50"},
                                        {"italic", "0"},
                                        {"Type", "QFont"},
                                        {"stylename", ""},
                                        {"bold", "0"},
                                        {"underline", "0"}});
                XML{"fontColor"}.attributes({{"Value", font_color}, {"Type", "QColor"}});

                XML{"alignment"}.attributes({{"Value", "129"}, {"Type", "enumAndFlags"}});
                XML{"allowHTML"}.attributes({{"Value", "1"}, {"Type", "bool"}});
                XML{"autoHeight"}.attributes({{"Value", "1"}, {"Type", "bool"}});
                XML{"backgroundMode"}.attributes({{"Value", "0"}, {"Type", "enumAndFlags"}});
                XML{"content"}
                    .attribute("Type", "QString")
                    .value("<b>" + field->get_description() + "</b>: $D{" + section_order.section->get_sql_section_name() + "_" + field->field_name +
                           ".Actual}");

                xml.writeStartElement("objectName");
                {
                    xml.writeAttribute("Type", "QString");
                    xml.writeCharacters("field_" + field->field_name);
                }
                xml.writeEndElement(); //objectName
            }
            xml.writeEndElement(); //item
            col++;
            if (col >= section_order.print_order_item.text_field_column_count) {
                col = 0;
                y += text_field_height;
                y_start += text_field_height;
            }
        }
        if (col != 0) {
            y_start += text_field_height;
        }
    }
    return y_start;
}

int Data_engine::generate_static_text_field(QXmlStreamWriter &xml, int y_start, const QString static_text, TextFieldDataBandPlace actual_band_position) const {
    int y = y_start;
    int text_field_height = 50;
    int font_size = 10;
    QString font_color = "#000000";
    if (static_text == "") {
        return y_start;
    }
    if ((actual_band_position == TextFieldDataBandPlace::page_footer) || (actual_band_position == TextFieldDataBandPlace::page_header)) {
        text_field_height = 30;
        font_size = 6;
        font_color = "#a8a8a8";
    }

    QString field_name;
    switch (actual_band_position) {
        case TextFieldDataBandPlace::report_footer:
            field_name = "static_report_footer" + QString::number(y_start);
            break;
        case TextFieldDataBandPlace::report_header:
            field_name = "static_report_header";
            break;
        case TextFieldDataBandPlace::page_footer:
            field_name = "static_page_footer";
            break;
        case TextFieldDataBandPlace::page_header:
            field_name = "static_page_header";
            break;
        default:
            field_name = "static_text" + QString::number(y_start);
            break;
    }

    const int x_position = 0;
    const int text_field_width = 2000;

    xml.writeStartElement("item");
    {
        xml.writeAttribute("Type", "Object");
        xml.writeAttribute("ClassName", "TextItem");
        XML{"geometry"}.attributes({{"width", QString::number(text_field_width)},
                                    {"height", QString::number(text_field_height)},
                                    {"x", QString::number(x_position)},
                                    {"y", QString::number(y)},
                                    {"Type", "QRect"}});
        XML{"font"}.attributes({{"pointSize", QString::number(font_size)},
                                {"family", "Arial"},
                                {"weight", "50"},
                                {"italic", "0"},
                                {"Type", "QFont"},
                                {"stylename", ""},
                                {"bold", "0"},
                                {"underline", "0"}});
        XML{"fontColor"}.attributes({{"Value", font_color}, {"Type", "QColor"}});

        XML{"alignment"}.attributes({{"Value", "129"}, {"Type", "enumAndFlags"}});
        XML{"allowHTML"}.attributes({{"Value", "1"}, {"Type", "bool"}});
        XML{"autoHeight"}.attributes({{"Value", "1"}, {"Type", "bool"}});
        XML{"backgroundMode"}.attributes({{"Value", "0"}, {"Type", "enumAndFlags"}});
        XML{"content"}.attribute("Type", "QString").value(static_text);

        xml.writeStartElement("objectName");
        {
            xml.writeAttribute("Type", "QString");
            xml.writeCharacters("field_" + field_name);
        }
        xml.writeEndElement(); //objectName
    }
    xml.writeEndElement(); //item

    y_start += text_field_height;

    return y_start;
}

void Data_engine::generate_pages_header(QXmlStreamWriter &xml, QString report_title, QString image_footer_path, QString image_header_path,
                                        QString approved_by_field_id, QString static_text_report_header, QString static_text_page_header,
                                        QString static_text_page_footer, QString static_text_report_footer_above_signature,
                                        QString static_text_report_footer_beneath_signature, const QList<PrintOrderItem> &print_order) const {
    {
        int y_position = 100;
        XML report_header{"item"};
        {
            report_header.attributes({{"Type", "Object"}, {"ClassName", "ReportHeader"}});
            report_header.add_band_index_element();
            XML{"objectName"}.attribute("Type", "QString").value("report_title");
            xml.writeStartElement("children");
            {
                xml.writeAttribute("Type", "Collection");
                xml.writeStartElement("item");
                {
                    xml.writeAttribute("Type", "Object");
                    xml.writeAttribute("ClassName", "TextItem");
                    XML{"geometry"}.attributes({{"width", "2000"}, {"height", QString::number(y_position)}, {"x", "0"}, {"y", "0"}, {"Type", "QRect"}});
                    XML{"font"}.attributes({{"pointSize", "20"},
                                            {"family", "Arial"},
                                            {"weight", "50"},
                                            {"italic", "0"},
                                            {"Type", "QFont"},
                                            {"stylename", ""},
                                            {"bold", "1"},
                                            {"underline", "0"}});
                    XML{"alignment"}.attributes({{"Value", "132"}, {"Type", "enumAndFlags"}});
                    XML{"content"}.attribute("Type", "QString").value(report_title);
                    xml.writeStartElement("objectName");
                    {
                        xml.writeAttribute("Type", "QString");
                        xml.writeCharacters("report_title");
                    }
                    xml.writeEndElement(); //objectName
                }
                xml.writeEndElement(); //item
                y_position += 100;
                y_position = generate_textfields(xml, y_position, print_order, TextFieldDataBandPlace::report_header);

                y_position = generate_static_text_field(xml, y_position, static_text_report_header, TextFieldDataBandPlace::report_header);
            }
            xml.writeEndElement(); //children
            XML{"geometry"}.attributes({{"width", "2000"}, {"height", QString::number(y_position)}, {"Type", "QRect"}});
            XML{"autoHeight"}.attributes({{"Value", "1"}, {"Type", "bool"}});
        }
    }
    {
        XML paget_Header{"item"};
        const QString header_name = "PageHeader_Band";
        {
            int y_position = 0;
            paget_Header.attributes({{"Type", "Object"}, {"ClassName", "PageHeader"}});
            paget_Header.add_band_index_element();
            xml.writeStartElement("objectName");
            {
                xml.writeAttribute("Type", "QString");
                xml.writeCharacters("PageFooter_Band");
            }
            xml.writeEndElement(); //objectName
            xml.writeStartElement("children");
            { //
                xml.writeAttribute("Type", "Collection");
                y_position = generate_image(xml, image_header_path, y_position, header_name);
                y_position = generate_textfields(xml, y_position, print_order, TextFieldDataBandPlace::page_header);
                y_position = generate_static_text_field(xml, y_position, static_text_page_header, TextFieldDataBandPlace::page_header);
            }
            xml.writeEndElement(); //children
            XML{"geometry"}.attributes({{"height", QString::number(y_position + 30)}, {"Type", "QRect"}, {"width", "2000"}});
            XML{"autoHeight"}.attributes({{"Value", "1"}, {"Type", "bool"}});
        }
    }
    {
        XML paget_footer{"item"};
        {
            int y_position = 0;
            const QString footer_name = "PageFooter_Band";
            paget_footer.attributes({{"Type", "Object"}, {"ClassName", "PageFooter"}});

            paget_footer.add_band_index_element();
            xml.writeStartElement("objectName");
            {
                xml.writeAttribute("Type", "QString");
                xml.writeCharacters("PageFooter_Band");
            }
            xml.writeEndElement(); //objectName
            xml.writeStartElement("children");
            {
                xml.writeAttribute("Type", "Collection");
                const int page_number_height = 50;

                y_position = generate_static_text_field(xml, y_position, static_text_page_footer, TextFieldDataBandPlace::page_footer);

                y_position = generate_textfields(xml, y_position, print_order, TextFieldDataBandPlace::page_footer);

                xml.writeStartElement("item");
                {
                    xml.writeAttribute("ClassName", "TextItem");
                    {
                        XML{"geometry"}.attributes({{"width", "2000"},
                                                    {"height", QString::number(page_number_height)},
                                                    {"x", "0"},
                                                    {"y", QString::number(y_position)},
                                                    {"Type", "QRect"}});
                        XML{"font"}.attributes({{"pointSize", "10"},
                                                {"family", "Arial"},
                                                {"weight", "50"},
                                                {"italic", "0"},
                                                {"Type", "QFont"},
                                                {"stylename", ""},
                                                {"underline", "0"}});
                        XML{"alignment"}.attributes({{"Value", "36"}, {"Type", "enumAndFlags"}});
                        XML{"content"}.attribute("Type", "QString").value("$V{#PAGE}/$V{#PAGE_COUNT}");
                        XML{"backgroundMode"}.attributes({{"Value", "0"}, {"Type", "enumAndFlags"}});
                        xml.writeStartElement("objectName");
                        {
                            xml.writeAttribute("Type", "QString");
                            xml.writeCharacters("Pagenumber");
                        }
                        xml.writeEndElement(); //objectName
                    }
                }
                xml.writeEndElement(); //item
                y_position += page_number_height;
                y_position = generate_image(xml, image_footer_path, y_position, footer_name);
            }
            xml.writeEndElement(); //children
            XML{"geometry"}.attributes({{"height", QString::number(y_position)}, {"Type", "QRect"}, {"width", "2000"}});
            XML{"autoHeight"}.attributes({{"Value", "1"}, {"Type", "bool"}});
        }
    }
    {
        XML report_header{"item"};
        {
            int y_position = 0;
            report_header.attributes({{"Type", "Object"}, {"ClassName", "ReportFooter"}});
            report_header.add_band_index_element();
            XML{"objectName"}.attribute("Type", "QString").value("report_footer");
            xml.writeStartElement("children");
            {
                xml.writeAttribute("Type", "Collection");
                y_position = generate_textfields(xml, y_position, print_order, TextFieldDataBandPlace::report_footer);

                y_position = generate_static_text_field(xml, y_position, static_text_report_footer_above_signature, TextFieldDataBandPlace::report_footer);

                y_position += 300;

                xml.writeStartElement("item");
                {
                    xml.writeAttribute("ClassName", "ShapeItem");
                    { XML{"geometry"}.attributes({{"width", "1000"}, {"height", "50"}, {"x", "500"}, {"y", QString::number(y_position)}, {"Type", "QRect"}}); }
                }
                xml.writeEndElement(); //ShapeItem

                xml.writeStartElement("item");
                {
                    xml.writeAttribute("ClassName", "TextItem");
                    {
                        XML{"geometry"}.attributes(
                            {{"width", "200"}, {"height", "50"}, {"x", "1300"}, {"y", QString::number(y_position + 25)}, {"Type", "QRect"}});
                        XML{"font"}.attributes({{"pointSize", "10"},
                                                {"family", "Arial"},
                                                {"weight", "50"},
                                                {"italic", "0"},
                                                {"Type", "QFont"},
                                                {"stylename", ""},
                                                {"underline", "0"}});
                        XML{"alignment"}.attributes({{"Value", "34"}, {"Type", "enumAndFlags"}});
                        XML{"content"}.attribute("Type", "QString").value(QObject::tr("Signature"));
                        XML{"backgroundMode"}.attributes({{"Value", "0"}, {"Type", "enumAndFlags"}});
                        XML{"autoHeight"}.attributes({{"Value", "1"}, {"Type", "bool"}});
                        xml.writeStartElement("objectName");
                        {
                            xml.writeAttribute("Type", "QString");
                            xml.writeCharacters("text_signature");
                        }
                        xml.writeEndElement(); //objectName
                    }
                }
                xml.writeEndElement(); //item

                xml.writeStartElement("item");
                {
                    xml.writeAttribute("ClassName", "TextItem");
                    {
                        XML{"geometry"}.attributes({{"width", "200"}, {"height", "50"}, {"x", "40"}, {"y", QString::number(y_position)}, {"Type", "QRect"}});
                        XML{"font"}.attributes({{"pointSize", "10"},
                                                {"family", "Arial"},
                                                {"weight", "50"},
                                                {"italic", "0"},
                                                {"Type", "QFont"},
                                                {"stylename", ""},
                                                {"underline", "0"}});
                        XML{"alignment"}.attributes({{"Value", "36"}, {"Type", "enumAndFlags"}});
                        XML{"content"}.attribute("Type", "QString").value("$S{dateFormat(now())}");
                        XML{"backgroundMode"}.attributes({{"Value", "0"}, {"Type", "enumAndFlags"}});
                        XML{"autoHeight"}.attributes({{"Value", "1"}, {"Type", "bool"}});
                        xml.writeStartElement("objectName");
                        {
                            xml.writeAttribute("Type", "QString");
                            xml.writeCharacters("text_signature_date");
                        }
                        xml.writeEndElement(); //objectName
                    }
                }
                xml.writeEndElement(); //item

                auto section_with_approved_by = sections.get_section_no_exception(approved_by_field_id);
                QString approved_query = "";
                if (section_with_approved_by) {
                    approved_query = "$D{" + section_with_approved_by->get_sql_section_name() + "_" +
                                     adjust_sql_table_name(approved_by_field_id.split("/")[1]) + "_automatic.Actual}";
                }
                xml.writeStartElement("item");
                {
                    xml.writeAttribute("ClassName", "TextItem");
                    {
                        XML{"geometry"}.attributes({{"width", "250"}, {"height", "50"}, {"x", "250"}, {"y", QString::number(y_position)}, {"Type", "QRect"}});
                        XML{"font"}.attributes({{"pointSize", "10"},
                                                {"family", "Arial"},
                                                {"weight", "50"},
                                                {"italic", "0"},
                                                {"Type", "QFont"},
                                                {"stylename", ""},
                                                {"underline", "0"}});
                        XML{"alignment"}.attributes({{"Value", "36"}, {"Type", "enumAndFlags"}});
                        XML{"content"}.attribute("Type", "QString").value(QObject::tr("Test operated by: ") + approved_query);
                        XML{"backgroundMode"}.attributes({{"Value", "0"}, {"Type", "enumAndFlags"}});
                        XML{"autoHeight"}.attributes({{"Value", "1"}, {"Type", "bool"}});
                        xml.writeStartElement("objectName");
                        {
                            xml.writeAttribute("Type", "QString");
                            xml.writeCharacters("text_tester");
                        }
                        xml.writeEndElement(); //objectName
                    }
                }
                xml.writeEndElement(); //item

                y_position += 50 + 25;

                y_position = generate_static_text_field(xml, y_position, static_text_report_footer_beneath_signature, TextFieldDataBandPlace::report_footer);
            }

            xml.writeEndElement(); //children
            XML{"geometry"}.attributes({{"width", "2000"}, {"height", QString::number(y_position)}, {"Type", "QRect"}});
            XML{"autoHeight"}.attributes({{"Value", "1"}, {"Type", "bool"}});
        }
    }
}

QList<PrintOrderSectionItem> Data_engine::get_print_order(const QList<PrintOrderItem> &orig_print_order, bool used_for_textfields,
                                                          TextFieldDataBandPlace actual_band_position) const {
    QList<PrintOrderItem> print_order_items;
    QStringList section_names_in_engine = get_section_names();
    for (auto order_item : orig_print_order) {
        if (order_item.print_enabled) {
            if ((used_for_textfields || order_item.print_as_text_field) == false) {
                print_order_items.append(order_item);
            } else if (used_for_textfields && order_item.print_as_text_field) {
                if ((actual_band_position == order_item.text_field_place) || (actual_band_position == TextFieldDataBandPlace::all)) {
                    print_order_items.append(order_item);
                }
            }
        }
        section_names_in_engine.removeAll(order_item.section_name);
    }

    if (used_for_textfields == false) {
        for (auto name : section_names_in_engine) {
            PrintOrderItem item{};
            item.print_enabled = true;
            item.section_name = name;
            item.print_as_text_field = false;
            print_order_items.append(item);
        }
    }

    QList<PrintOrderSectionItem> result;
    for (auto print_order_item : print_order_items) {
        DataEngineSection *section = sections.get_section(print_order_item.section_name + "/dummy");
        assert(section);
        PrintOrderSectionItem item;
        item.print_order_item = print_order_item;
        item.section = section;
        result.append(item);
    }
    return result;
}

void Data_engine::generate_tables(const QList<PrintOrderItem> &print_order) const {
    auto section_print_order = get_print_order(print_order, false, TextFieldDataBandPlace::none);

    for (auto section_print_order_item : section_print_order) {
        generate_table(section_print_order_item.section);
    }
    generate_exception_approval_table();
}

void Data_engine::generate_exception_approval_table() const {
    const auto &headers = {QObject::tr("ID"), QObject::tr("Description"), QObject::tr("Approved by"), QObject::tr("Signature")};
    const int column_widths[4] = {5, 40, 25, 30};
    const auto &sql_fields = {"ID", "Description", "approving_person", ""};
    //Data Band Header
    {
        XML data_band_header{"item"};
        data_band_header.attributes({{"Type", "Object"}, {"ClassName", "DataHeader"}});
        data_band_header.add_band_index_element();
        XML{"parentBand"}.attribute("Type", "QString").value(QString{"DataBand%1"}.arg(exceptional_approvals_table_name));
        XML{"geometry"}.attributes({{"width", "2000"}, {"height", "170"}, {"x", "0"}, {"y", QString::number(XML::y() += 100)}, {"Type", "QRect"}});
        XML{"objectName"}.attribute("Type", "QString").value(QString{"DataBandHeader_exceptional_approval"});
        XML{"printAlways"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"splittable"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        {
            XML children{"children"};
            children.attribute("Type", "Collection");
            {
                {
                    XML title{"item"};
                    title.attribute("Type", "Object");
                    title.attribute("ClassName", "TextItem");
                    XML{"alignment"}.attributes({{"Value", "129"}, {"Type", "enumAndFlags"}});
                    XML{"font"}.attributes({{"Value", "129"},
                                            {"Type", "QFont"},
                                            {"stylename", ""},
                                            {"weight", "50"},
                                            {"family", "Arial"},
                                            {"pointSize", "16"},
                                            {"underline", "0"},
                                            {"italic", "0"},
                                            {"bold", "1"}});
                    XML{"geometry"}.attributes({{"width", "2000"}, {"height", "100"}, {"x", "0"}, {"y", "50"}, {"Type", "QRect"}});
                    XML{"alignment"}.attributes({{"Value", "129"}, {"Type", "enumAndFlags"}});
                    XML{"content"}.attribute("Type", "QString").value(QObject::tr("Exceptional Approvals"));
                    XML{"objectName"}.attribute("Type", "QString").value(QString{"DataHeaderTitle_exceptional_approval"});
                }
                int x_position = 0;
                for (std::size_t i = 0; i < headers.size(); i++) {
                    const int field_width = 2000 * column_widths[i] / 100;
                    XML column_header{"item"};
                    column_header.attribute("Type", "Object");
                    column_header.attribute("ClassName", "TextItem");
                    const auto &header = headers.begin()[i];
                    XML{"geometry"}.attributes(
                        {{"width", QString::number(field_width)}, {"height", "50"}, {"x", QString::number(x_position)}, {"y", "130"}, {"Type", "QRect"}});
                    XML{"alignment"}.attributes({{"Value", "129"}, {"Type", "enumAndFlags"}});
                    XML{"font"}.attributes({{"Value", "129"},
                                            {"Type", "QFont"},
                                            {"stylename", ""},
                                            {"weight", "50"},
                                            {"family", "Arial"},
                                            {"pointSize", "10"},
                                            {"underline", "0"},
                                            {"italic", "0"},
                                            {"bold", "1"}});
                    XML{"content"}.attribute("Type", "QString").value(header);
                    XML{"objectName"}.attribute("Type", "QString").value(QString{"DataHeaderColumnHeader_%1_exceptional_approval"}.arg(header));
                    x_position += field_width;
                }
            }
        }
    }
    //Data Band
    {
        XML data_band{"item"};
        data_band.attributes({{"Type", "Object"}, {"ClassName", "Data"}});
        data_band.add_band_index_element();
        XML{"geometry"}.attributes({{"width", "2000"}, {"height", "80"}, {"x", "0"}, {"y", QString::number(XML::y() += 100)}, {"Type", "QRect"}});
        XML{"objectName"}.attribute("Type", "QString").value(QString{"DataBand%1"}.arg(exceptional_approvals_table_name));
        XML{"datasource"}.attribute("Type", "QString").value(exceptional_approvals_table_name);
        XML{"splittable"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"keepBottomSpace"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"printIfEmpty"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"keepFooterTogether"}.attributes({{"Value", "0"}, {"Type", "bool"}});

        {
            int x_position = 0;
            const int row_height = 70;
            {
                XML children{"children"};
                children.attribute("Type", "Collection");
                for (std::size_t i = 0; i < headers.size() - 1; i++) {
                    const int field_width = 2000 * column_widths[i] / 100;
                    XML column{"item"};
                    column.attribute("Type", "Object");
                    column.attribute("ClassName", "TextItem");
                    const auto &header = headers.begin()[i];

                    XML{"geometry"}.attributes({{"width", QString::number(field_width)},
                                                {"height", QString::number(row_height)},
                                                {"x", QString::number(x_position)},
                                                {"y", "0"},
                                                {"Type", "QRect"}});
                    XML{"alignment"}.attributes({{"Value", "129"}, {"Type", "enumAndFlags"}});
                    XML{"autoHeight"}.attributes({{"Value", "1"}, {"Type", "bool"}});
                    XML{"backgroundMode"}.attributes({{"Value", "0"}, {"Type", "enumAndFlags"}});
                    XML{"content"}.attribute("Type", "QString").value(QString{"$D{%1.%2}"}.arg(exceptional_approvals_table_name, sql_fields.begin()[i]));
                    XML{"objectName"}.attribute("Type", "QString").value(QString{} + "DataHeaderColumn" + header + exceptional_approvals_table_name);
                    x_position += field_width;
                }
                const int field_width = 2000 * column_widths[3] / 100;
                {
                    XML item{"item"};
                    {
                        item.attribute("ClassName", "ShapeItem");
                        {
                            XML{"geometry"}.attributes({{"width", QString::number(field_width)},
                                                        {"height", "-1"},
                                                        {"x", QString::number(x_position)},
                                                        {"y", QString::number(row_height - 1)},
                                                        {"Type", "QRect"}});
                        }
                    }
                }
            }
        }
    }

    //Data Band Footer
    {
        XML data_footer{"item"};
        data_footer.attributes({{"Type", "Object"}, {"ClassName", "DataFooter"}});
        data_footer.add_band_index_element();
        XML{"parentBand"}.attribute("Type", "QString").value(QString{"DataBand%1"}.arg(exceptional_approvals_table_name));
        XML{"geometry"}.attributes({{"width", "2000"}, {"height", "50"}, {"x", "0"}, {"y", QString::number(XML::y() += 100)}, {"Type", "QRect"}});
        XML{"objectName"}.attribute("Type", "QString").value(QString{"DataFooter%1"}.arg(exceptional_approvals_table_name));
        XML{"splittable"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"keepBottomSpace"}.attributes({{"Value", "1"}, {"Type", "bool"}});

        XML{"keepFooterTogether"}.attributes({{"Value", "1"}, {"Type", "bool"}});
        XML{"autoHeight"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"printIfEmpty"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"printAlways"}.attributes({{"Value", "0"}, {"Type", "bool"}});
    }
}

void Data_engine::generate_table(const DataEngineSection *section) const {
    const auto &headers = {QObject::tr("Name"), QObject::tr("Target Value"), QObject::tr("Actual Value"), QObject::tr("Result")};
    const int column_widths[4] = {40, 20, 20, 20};
    const auto &sql_fields = {"Description", "Desired", "Actual", "Inrange"};
    //Data Band Header
    {
        XML data_band_header{"item"};
        data_band_header.attributes({{"Type", "Object"}, {"ClassName", "DataHeader"}});
        data_band_header.add_band_index_element();
        XML{"parentBand"}.attribute("Type", "QString").value(QString{"DataBand%1"}.arg(section->get_sql_section_name()));
        XML{"geometry"}.attributes({{"width", "2000"}, {"height", "170"}, {"x", "0"}, {"y", QString::number(XML::y() += 100)}, {"Type", "QRect"}});
        XML{"objectName"}.attribute("Type", "QString").value(QString{"DataBandHeader_%1"}.arg(section->get_sql_section_name()));
        XML{"printAlways"}.attributes({{"Value", "1"}, {"Type", "bool"}});
        XML{"splittable"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        {
            XML children{"children"};
            children.attribute("Type", "Collection");
            {
                {
                    XML title{"item"};
                    title.attribute("Type", "Object");
                    title.attribute("ClassName", "TextItem");
                    XML{"alignment"}.attributes({{"Value", "129"}, {"Type", "enumAndFlags"}});
                    XML{"font"}.attributes({{"Value", "129"},
                                            {"Type", "QFont"},
                                            {"stylename", ""},
                                            {"weight", "50"},
                                            {"family", "Arial"},
                                            {"pointSize", "16"},
                                            {"underline", "0"},
                                            {"italic", "0"},
                                            {"bold", "1"}});
                    XML{"geometry"}.attributes({{"width", "2000"}, {"height", "100"}, {"x", "0"}, {"y", "50"}, {"Type", "QRect"}});
                    XML{"alignment"}.attributes({{"Value", "129"}, {"Type", "enumAndFlags"}});
                    XML{"content"}.attribute("Type", "QString").value(section->get_section_title());
                    XML{"objectName"}.attribute("Type", "QString").value(QString{"DataHeaderTitle_%1"}.arg(section->get_sql_section_name()));
                }
                int x_position = 0;
                for (std::size_t i = 0; i < headers.size(); i++) {
                    const int field_width = 2000 * column_widths[i] / 100;
                    XML column_header{"item"};
                    column_header.attribute("Type", "Object");
                    column_header.attribute("ClassName", "TextItem");
                    const auto &header = headers.begin()[i];
                    XML{"geometry"}.attributes(
                        {{"width", QString::number(field_width)}, {"height", "50"}, {"x", QString::number(x_position)}, {"y", "130"}, {"Type", "QRect"}});
                    XML{"alignment"}.attributes({{"Value", "129"}, {"Type", "enumAndFlags"}});
                    XML{"font"}.attributes({{"Value", "129"},
                                            {"Type", "QFont"},
                                            {"stylename", ""},
                                            {"weight", "50"},
                                            {"family", "Arial"},
                                            {"pointSize", "10"},
                                            {"underline", "0"},
                                            {"italic", "0"},
                                            {"bold", "1"}});
                    XML{"content"}.attribute("Type", "QString").value(header);
                    XML{"objectName"}.attribute("Type", "QString").value(QString{"DataHeaderColumnHeader_%1_%2"}.arg(header, section->get_sql_instance_name()));
                    x_position += field_width;
                }
            }
        }
    }
    //Data Band
    {
        XML data_band{"item"};
        data_band.attributes({{"Type", "Object"}, {"ClassName", "Data"}});
        data_band.add_band_index_element();
        XML{"geometry"}.attributes({{"width", "2000"}, {"height", "80"}, {"x", "0"}, {"y", QString::number(XML::y() += 100)}, {"Type", "QRect"}});
        XML{"objectName"}.attribute("Type", "QString").value(QString{"DataBand%1"}.arg(section->get_sql_section_name()));
        XML{"datasource"}.attribute("Type", "QString").value(section->get_sql_instance_name());
        XML{"splittable"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"keepBottomSpace"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"printIfEmpty"}.attributes({{"Value", "1"}, {"Type", "bool"}});
        XML{"keepFooterTogether"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"keepSubdetailTogether"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        {
            XML children{"children"};
            children.attribute("Type", "Collection");
            {
                XML column{"item"};
                column.attribute("Type", "Object");
                column.attribute("ClassName", "TextItem");
                XML{"geometry"}.attributes({{"width", "2000"}, {"height", "50"}, {"x", "0"}, {"y", "20"}, {"Type", "QRect"}});
                XML{"alignment"}.attributes({{"Value", "129"}, {"Type", "enumAndFlags"}});
                XML{"font"}.attributes({{"Value", "129"},
                                        {"Type", "QFont"},
                                        {"stylename", ""},
                                        {"weight", "50"},
                                        {"family", "Arial"},
                                        {"pointSize", "10"},
                                        {"underline", "0"},
                                        {"italic", "1"},
                                        {"bold", "0"}});
                XML{"content"}.attribute("Type", "QString").value(QString{"$D{%1.%2}"}.arg(section->get_sql_instance_name(), "Caption"));
                XML{"objectName"}.attribute("Type", "QString").value("DataSectionName_" + section->get_sql_section_name());
            }
        }
    }
    //Data Band Subdetail
    {
        XML data_subdetail_header{"item"};
        data_subdetail_header.attributes({{"Type", "Object"}, {"ClassName", "SubDetail"}});
        data_subdetail_header.add_band_index_element();
        XML{"parentBand"}.attribute("Type", "QString").value(QString{"DataBand%1"}.arg(section->get_sql_section_name()));
        XML{"geometry"}.attributes({{"width", "2000"}, {"height", "50"}, {"x", "0"}, {"y", QString::number(XML::y() += 100)}, {"Type", "QRect"}});
        XML{"objectName"}.attribute("Type", "QString").value(QString{"SubDetail%1"}.arg(section->get_sql_section_name()));
        XML{"datasource"}.attribute("Type", "QString").value(section->get_sql_section_name());
        XML{"splittable"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"alternateBackgroundColor"}.attributes({{"Value", "#F2F2F2"}, {"Type", "QColor"}});
        XML{"keepBottomSpace"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"keepFooterTogether"}.attributes({{"Value", "0"}, {"Type", "bool"}});

        {
            XML children{"children"};
            children.attribute("Type", "Collection");
            int x_position = 0;
            for (std::size_t i = 0; i < headers.size(); i++) {
                const int field_width = 2000 * column_widths[i] / 100;
                XML column{"item"};
                column.attribute("Type", "Object");
                column.attribute("ClassName", "TextItem");
                const auto &header = headers.begin()[i];

                XML{"geometry"}.attributes(
                    {{"width", QString::number(field_width)}, {"height", "50"}, {"x", QString::number(x_position)}, {"y", "0"}, {"Type", "QRect"}});
                XML{"alignment"}.attributes({{"Value", "129"}, {"Type", "enumAndFlags"}});
                XML{"autoHeight"}.attributes({{"Value", "1"}, {"Type", "bool"}});
                XML{"backgroundMode"}.attributes({{"Value", "0"}, {"Type", "enumAndFlags"}});
                XML{"content"}.attribute("Type", "QString").value(QString{"$D{%1.%2}"}.arg(section->get_sql_section_name(), sql_fields.begin()[i]));
                XML{"objectName"}.attribute("Type", "QString").value(QString{} + "DataHeaderColumn" + header + section->get_sql_section_name());
                x_position += field_width;
            }
        }
    }
    //Sub Data Band Footer
    {
        XML data_footer{"item"};
        data_footer.attributes({{"Type", "Object"}, {"ClassName", "SubDetailFooter"}});
        data_footer.add_band_index_element();
        XML{"parentBand"}.attribute("Type", "QString").value(QString{"SubDetail%1"}.arg(section->get_sql_section_name()));
        XML{"geometry"}.attributes({{"width", "2000"}, {"height", "25"}, {"x", "0"}, {"y", QString::number(XML::y() += 100)}, {"Type", "QRect"}});
        XML{"objectName"}.attribute("Type", "QString").value(QString{"SubDetailFooter%1"}.arg(section->get_sql_section_name()));
        XML{"splittable"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"keepBottomSpace"}.attributes({{"Value", "1"}, {"Type", "bool"}});

        XML{"keepFooterTogether"}.attributes({{"Value", "1"}, {"Type", "bool"}});
        XML{"autoHeight"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"printIfEmpty"}.attributes({{"Value", "1"}, {"Type", "bool"}});
        XML{"printAlways"}.attributes({{"Value", "1"}, {"Type", "bool"}});
    }
    //Data Band Footer
    {
        XML data_footer{"item"};
        data_footer.attributes({{"Type", "Object"}, {"ClassName", "DataFooter"}});
        data_footer.add_band_index_element();
        XML{"parentBand"}.attribute("Type", "QString").value(QString{"DataBand%1"}.arg(section->get_sql_section_name()));
        XML{"geometry"}.attributes({{"width", "2000"}, {"height", "50"}, {"x", "0"}, {"y", QString::number(XML::y() += 100)}, {"Type", "QRect"}});
        XML{"objectName"}.attribute("Type", "QString").value(QString{"DataFooter%1"}.arg(section->get_sql_section_name()));
        XML{"splittable"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"keepBottomSpace"}.attributes({{"Value", "1"}, {"Type", "bool"}});

        XML{"keepFooterTogether"}.attributes({{"Value", "1"}, {"Type", "bool"}});
        XML{"autoHeight"}.attributes({{"Value", "0"}, {"Type", "bool"}});
        XML{"printIfEmpty"}.attributes({{"Value", "1"}, {"Type", "bool"}});
        XML{"printAlways"}.attributes({{"Value", "1"}, {"Type", "bool"}});
    }
}

void Data_engine::replace_database_filename(const std::string &source_form_path, const std::string &destination_form_path, const std::string &database_path) {
    QFile xml_file_in{source_form_path.c_str()};
    xml_file_in.open(QFile::OpenModeFlag::ReadOnly);
    assert(xml_file_in.isOpen());
    QXmlStreamReader xml_in{&xml_file_in};
    QFile xml_file_out{destination_form_path.c_str()};
    xml_file_out.open(QFile::OpenModeFlag::WriteOnly | QFile::OpenModeFlag::Truncate);
    assert(xml_file_out.isOpen());
    qDebug() << "xml_file_in" << xml_file_in.fileName();
    qDebug() << "xml_file_out" << xml_file_out.fileName();
    QXmlStreamWriter &xml_out = XML::state.xml;
    xml_out.setDevice(&xml_file_out);
    while ((!xml_in.atEnd())) {
        xml_in.readNext();

        if (xml_in.isStartElement()) {
            if (xml_in.name() == "databaseName") {
                XML{"databaseName"}.attribute("Type", "QString").value(QString::fromStdString(database_path));
                xml_in.skipCurrentElement();
                continue;
            }
        }
        xml_out.writeCurrentToken(xml_in);
    }
    if (xml_in.hasError()) {
        qDebug() << "XML Error:" << xml_in.errorString();
    }
    // assert(!xml_in.hasError());
}

void Data_engine::add_sources_to_form(QString data_base_path, const QList<PrintOrderItem> &print_order, QString approved_by_field_id) const {
    XML{"objectName"}.attribute("Type", "QString").value("datasources");
    //connections
    data_base_path = QDir::toNativeSeparators(data_base_path);
    const auto connection_name = "default_connection";
    {
        XML connections{"connections"};
        connections.attribute("Type", "Collection");
        XML item{"item"};
        item.attributes({{"ClassName", "LimeReport::ConnectionDesc"}, {"Type", "Object"}});
        XML{"name"}.attribute("Type", "QString").value(connection_name);
        XML{"driver"}.attribute("Type", "QString").value("QSQLITE");
        XML{"databaseName"}.attribute("Type", "QString").value(data_base_path);
    }
    //queries
    {
        XML queries{"queries"};
        queries.attribute("Type", "Collection");
        for (const auto &section : sections.sections) {
            XML item{"item"};
            item.attributes({{"ClassName", "LimeReport::QueryDesc"}, {"Type", "Object"}});
            XML{"queryName"}.attribute("Type", "QString").value(section.get_sql_instance_name());
            XML{"queryText"}.attribute("Type", "QString").value(QString{"SELECT * FROM %1"}.arg(section.get_sql_instance_name()));
            XML{"connectionName"}.attribute("Type", "QString").value(connection_name);
        }

        QList<PrintOrderSectionItem> textfield_print_order = get_print_order(print_order, true, TextFieldDataBandPlace::all);

        PrintOrderSectionItem approved_id;
        DataEngineSection *approved_section = sections.get_section_no_exception(approved_by_field_id);
        if (approved_section) {
            approved_id.section = approved_section;
            approved_id.field_name = adjust_sql_table_name(approved_by_field_id.split("/")[1]);
            approved_id.suffix = "_automatic";
            textfield_print_order.append(approved_id);
        }

        for (const auto &section_item : textfield_print_order) {
            assert(section_item.section->instances.size() == 1);
            const DataEngineInstance &instance = section_item.section->instances[0];
            auto variant = instance.get_variant();
            assert(variant);

            QStringList field_names;
            if (section_item.field_name.count()) {
                field_names.append(section_item.field_name);
            } else {
                for (const std::unique_ptr<DataEngineDataEntry> &entry : variant->data_entries) {
                    field_names.append(entry->field_name);
                }
            }
            for (auto &field_name : field_names) {
                XML item{"item"};
                item.attributes({{"ClassName", "LimeReport::QueryDesc"}, {"Type", "Object"}});

                XML{"queryName"}.attribute("Type", "QString").value(section_item.section->get_sql_section_name() + "_" + field_name + section_item.suffix);
                XML{"queryText"}
                    .attribute("Type", "QString")
                    .value(QString{"SELECT * FROM %1 WHERE Name = \"%2\""}
                               .arg(section_item.section->get_sql_section_name())
                               .arg(section_item.section->get_section_name() + "/" + field_name));
                XML{"connectionName"}.attribute("Type", "QString").value(connection_name);
            }
        }
        {
            XML item{"item"};
            item.attributes({{"ClassName", "LimeReport::QueryDesc"}, {"Type", "Object"}});

            XML{"queryName"}.attribute("Type", "QString").value(exceptional_approvals_table_name);
            XML{"queryText"}.attribute("Type", "QString").value(QString{"SELECT * FROM %1 ORDER BY ID"}.arg(exceptional_approvals_table_name));
            XML{"connectionName"}.attribute("Type", "QString").value(connection_name);
        }
    }

    //subqueries
    {
        XML subqueries{"subqueries"};
        subqueries.attribute("Type", "Collection");
        for (const auto &section : sections.sections) {
            XML item{"item"};
            item.attributes({{"ClassName", "LimeReport::SubQueryDesc"}, {"Type", "Object"}});
            XML{"queryName"}.attribute("Type", "QString").value(section.get_sql_section_name());
            XML{"queryText"}
                .attribute("Type", "QString")
                .value(QString{"SELECT * FROM %1 WHERE InstanceID = $D{%2.InstanceID}"}.arg(section.get_sql_section_name(), section.get_sql_instance_name()));
            XML{"connectionName"}.attribute("Type", "QString").value(connection_name);
            XML{"master"}.attribute("Type", "QString").value(section.get_sql_instance_name());
        }
    }
}

void Data_engine::assert_in_dummy_mode() const {
    if (sections.is_dummy_data_mode) {
        throw DataEngineError(DataEngineErrorNumber::is_in_dummy_mode, "Dataengine: Dataengine is in dummy mode. Access to data functions is prohibited.");
    }
}

bool Data_engine::entry_compare(const FormIdWrapper &lhs, const FormIdWrapper &rhs) {
    return lhs.value < rhs.value;
}

double NumericTolerance::get_absolute_limit_beneath(const double desired) const {
    if (open_range_beneath) {
        return std::numeric_limits<double>::lowest();
    }
    switch (tolerance_type) {
        case ToleranceType::Absolute: {
            return desired - deviation_limit_beneath;
        } break;
        case ToleranceType::Percent: {
            double deviation_limit_abs = std::abs(desired) * deviation_limit_beneath / 100.0;
            return desired - deviation_limit_abs;
        }

        break;
    }
    assert(0);
    return 0;
}

double NumericTolerance::get_absolute_limit_above(const double desired) const {
    if (open_range_above) {
        return std::numeric_limits<double>::max();
    }

    switch (tolerance_type) {
        case ToleranceType::Absolute: {
            return desired + deviation_limit_above;

        } break;
        case ToleranceType::Percent: {
            double deviation_limit_abs = std::abs(desired) * deviation_limit_beneath / 100.0;
            return desired + deviation_limit_abs;

        }

        break;
    }
    assert(0);
    return 0;
}

bool NumericTolerance::test_in_range(const double desired, const std::experimental::optional<double> &measured) const {
    double min_value_absolute = 0;
    double max_value_absolute = 0;

    if (is_undefined) {
        throw DataEngineError(DataEngineErrorNumber::tolerance_must_be_defined_for_range_checks_on_numbers,
                              "Dataengine: no tolerance defined even though a range check should be done.");
    }
    if (!measured) {
        return false;
    }

    if (std::isnan(measured.value())) { //fixes #1113 Fehler: "Dataengine: NaN ist immer OK, soll natürlich nicht"
        return false;
    }

    max_value_absolute = get_absolute_limit_above(desired);
    min_value_absolute = get_absolute_limit_beneath(desired);

    return (min_value_absolute <= measured) && (measured <= max_value_absolute);
}

void NumericTolerance::from_string(const QString &str) {
    QStringList sl = str.split("/");
    if (sl.count() == 0) {
        throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error, "Dataengine: no tolerance found");
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
            throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error,
                                  "Dataengine: tolerance + and - must of the same type(either absolute or percent)");
        }
        open_range_above = open_range_a;
        open_range_beneath = open_range_b;
        is_undefined = false;
    } else {
        throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error, "Dataengine: tolerance string faulty");
    }
}

QString NumericTolerance::to_string(const double desired_value) const {
    QString result;

    if (open_range_above && open_range_beneath) {
        result = num_to_str(desired_value, ToleranceType::Absolute) + " (±∞)";
    } else if (open_range_beneath) {
        result = "≤ " + num_to_str(desired_value, ToleranceType::Absolute);
        if (deviation_limit_above > 0.0) {
            result += " (+" + num_to_str(deviation_limit_above, tolerance_type) + ")";
        }
    } else if (open_range_above) {
        result = "≥ " + num_to_str(desired_value, ToleranceType::Absolute);
        if (deviation_limit_beneath > 0.0) {
            result += " (-" + num_to_str(deviation_limit_beneath, tolerance_type) + ")";
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

QJsonObject NumericTolerance::get_json(double desirec_value) const {
    QJsonObject result;
    QJsonObject above;
    QJsonObject beneath;
    above["open_range"] = open_range_above;
    if (!open_range_above) {
        above["limit"] = get_absolute_limit_above(desirec_value);
    }
    beneath["open_range"] = open_range_beneath;
    if (!open_range_beneath) {
        beneath["limit"] = get_absolute_limit_beneath(desirec_value);
    }
    result["above"] = above;
    result["beneath"] = beneath;
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
        if ((sign == "") && ((str_in[0].isDigit()) || (str_in[0] == '*'))) {
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
            throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error,
                                  QString("Dataengine: could not convert string to number %1").arg(str_in_orig));
        }
    }
    if (sign_error) {
        throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error,
                              QString("Dataengine: tolerance string should have sign flags %1").arg(str_in_orig));
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

void DataEngineDataEntry::set_exceptional_approval(ExceptionalApprovalResult exceptional_approval) {
    this->exceptional_approval = exceptional_approval;
}

const ExceptionalApprovalResult &DataEngineDataEntry::get_exceptional_approval() const {
    return exceptional_approval;
}

std::unique_ptr<DataEngineDataEntry> DataEngineDataEntry::from_json(const QJsonObject &object) {
    FormID field_name;
    EntryType entrytype = EntryType::Unspecified;
    const auto keys = object.keys();
    bool entry_without_value;
    if (!keys.contains("value")) {
        if (!keys.contains("type")) {
            throw DataEngineError(DataEngineErrorNumber::data_entry_contains_neither_type_nor_value,
                                  "Dataengine: JSON object must contain a key \"value\" or \"type\" ");
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
        throw DataEngineError(DataEngineErrorNumber::data_entry_contains_no_name, "Dataengine: JSON object must contain key \"name\"");
    }
    field_name = object.value("name").toString();
    switch (entrytype) {
        case EntryType::Numeric: {
//disable maybe-uninitialized warning for an optional type.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
            std::experimental::optional<double> desired_value{};
#pragma GCC diagnostic pop
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
                        throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error, "Dataengine: wrong tolerance type");
                    }

                    tolerance.from_string(tol);
                } else if (entry_without_value == false) {
                    throw DataEngineError(DataEngineErrorNumber::invalid_data_entry_key, "Dataengine: Invalid key \"" + key + "\" in numeric JSON object");

                } else {
                    throw DataEngineError(DataEngineErrorNumber::invalid_data_entry_key,
                                          "Dataengine: Invalid key \"" + key + "\" in numeric JSON object without desired value");
                }
            }
            if ((tolerance.is_defined() == false) && (entry_without_value == false)) {
                throw DataEngineError(DataEngineErrorNumber::tolerance_must_be_defined_for_numbers,
                                      "Dataengine: The number field with key \"" + field_name +
                                          "\" has no tolerance defined. Each number field must have a tolerance defined.");
            }
            return std::make_unique<NumericDataEntry>(field_name, desired_value, tolerance, std::move(unit), si_prefix, std::move(nice_name));
        }
        case EntryType::Bool: {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
            std::experimental::optional<bool> desired_value{};
#pragma GCC diagnostic pop

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
                    throw DataEngineError(DataEngineErrorNumber::invalid_data_entry_key, "Dataengine: Invalid key \"" + key + "\" in boolean JSON object");
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
                    throw DataEngineError(DataEngineErrorNumber::invalid_data_entry_key, "Dataengine: Invalid key \"" + key + "\" in textual JSON object");
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
                        throw DataEngineError(DataEngineErrorNumber::tolerance_parsing_error, "Dataengine: wrong tolerance type");
                    }

                    tolerance.from_string(tol);
                } else if (key == "value") {
                    reference_string = object.value(key).toString();

                } else {
                    throw DataEngineError(DataEngineErrorNumber::invalid_data_entry_key, "Dataengine: Invalid key \"" + key + "\" in reference JSON object");
                }
            }
            return std::make_unique<ReferenceDataEntry>(field_name, reference_string, tolerance, nice_name);
        }
        case EntryType::Unspecified: {
            throw DataEngineError(DataEngineErrorNumber::invalid_data_entry_type,
                                  QString("Dataengine: Invalid type in JSON object. Field name: %1").arg(field_name));
        }
    }
    throw DataEngineError(DataEngineErrorNumber::invalid_json_object, QString{"Dataengine: Invalid JSON object. Field name: %1"}.arg(field_name));
}

NumericDataEntry::NumericDataEntry(const NumericDataEntry &other)
    : DataEngineDataEntry(other.field_name)
    , desired_value{other.desired_value}
    , unit{other.unit}
    , description{other.description}
    , si_prefix{other.si_prefix}
    , tolerance{other.tolerance}
    , actual_value{other.actual_value} {}

NumericDataEntry::NumericDataEntry(FormID field_name, std::experimental::optional<double> desired_value, NumericTolerance tolerance, QString unit,
                                   std::experimental::optional<double> si_prefix, QString description)
    : DataEngineDataEntry(field_name)
    , desired_value(desired_value)
    , unit(std::move(unit))
    , description(std::move(description))
    , tolerance(tolerance) {
    this->si_prefix = si_prefix.value_or(1.0);
}

bool NumericDataEntry::is_complete() const {
    if ((bool)actual_value == false) {
        return false;
    }
    return true;
}

bool NumericDataEntry::is_in_range() const {
    if (is_complete() == false) {
        return false;
    }
    if ((bool)desired_value) {
        if (tolerance.test_in_range(desired_value.value(), actual_value) == false) {
            return false;
        }
    }
    return true;
}

QString NumericDataEntry::get_desired_value_as_string() const {
    if ((bool)desired_value) {
        QString val = tolerance.to_string(desired_value.value()) + " " + unit;
        return val.trimmed();
    } else {
        return "";
    }
}

QString NumericDataEntry::get_actual_values() const {
    if ((bool)actual_value) {
        QString val = QString::number(actual_value.value()) + " " + unit;
        return val.trimmed();
    } else {
        return unavailable_value;
    }
}

double NumericDataEntry::get_actual_number() const {
    if ((bool)actual_value) {
        return actual_value.value();
    } else {
        throw DataEngineError(DataEngineErrorNumber::actual_value_not_set, QString("Dataengine: Actual value of field %1 not set").arg(field_name));
    }
    return 0;
}

QString NumericDataEntry::get_description() const {
    return description;
}

QString NumericDataEntry::get_unit() const {
    return unit;
}

double NumericDataEntry::get_si_prefix() const {
    return si_prefix;
}

bool NumericDataEntry::compare_unit_desired_siprefix(const DataEngineDataEntry *from) const {
    auto const from_num = dynamic_cast<const NumericDataEntry *>(from);
    if (from_num == nullptr) {
        return false;
    }
    if (from_num->desired_value != desired_value) {
        return false;
    }
    if (from_num->unit != unit) {
        return false;
    }
    if (from_num->si_prefix != si_prefix) {
        return false;
    }
    return true;
}

void NumericDataEntry::set_actual_value(double actual_value) {
    this->actual_value = actual_value / si_prefix;
}

EntryType NumericDataEntry::get_entry_type() const {
    return EntryType::Numeric;
}

bool NumericDataEntry::is_desired_value_set() const {
    return (bool)desired_value;
}

QJsonObject NumericDataEntry::get_specific_json_dump() const {
    QJsonObject result;
    QJsonObject desired;
    QJsonObject actual;
    result["unit"] = get_unit();
    desired["desired_is_set"] = (bool)desired_value;
    if ((bool)desired_value) {
        desired["as_text"] = get_desired_value_as_string();
        desired["value"] = desired_value.value();
        desired["tolerance"] = tolerance.get_json(desired_value.value());
    }

    actual["actual_is_set"] = (bool)actual_value;
    if ((bool)actual_value) {
        actual["value"] = actual_value.value();
    }

    result["actual"] = actual;

    result["desired"] = desired;
    return result;
}

QString NumericDataEntry::get_specific_json_name() const {
    return "numeric";
}

void NumericDataEntry::set_desired_value_from_desired(DataEngineDataEntry *from) {
    NumericDataEntry *num_from = from->as<NumericDataEntry>();
    assert(num_from);
    desired_value = num_from->desired_value;
}

void NumericDataEntry::set_desired_value_from_actual(DataEngineDataEntry *from) {
    NumericDataEntry *num_from = from->as<NumericDataEntry>();
    assert(num_from);
    desired_value = num_from->actual_value;
}

TextDataEntry::TextDataEntry(const TextDataEntry &other)
    : DataEngineDataEntry{other.field_name}
    , desired_value{other.desired_value}
    , description{other.description}
    , actual_value{other.actual_value} {}

TextDataEntry::TextDataEntry(const FormID name, std::experimental::optional<QString> desired_value, QString description)
    : DataEngineDataEntry(name)
    , desired_value(std::move(desired_value))
    , description(description) {}

bool TextDataEntry::is_complete() const {
    if ((bool)actual_value == false) {
        return false;
    }
    return true;
}

bool TextDataEntry::is_in_range() const {
    if (is_complete() == false) {
        return false;
    }
    if ((bool)desired_value) {
        if (actual_value.value() != desired_value) {
            return false;
        }
    }
    return true;
}

QString TextDataEntry::get_actual_values() const {
    if ((bool)actual_value) {
        return actual_value.value();
    } else {
        return unavailable_value;
    }
}

double TextDataEntry::get_actual_number() const {
    throw DataEngineError(DataEngineErrorNumber::actual_value_is_not_a_number, QString("Dataengine: Actual value of field %1 is not a number").arg(field_name));
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

double TextDataEntry::get_si_prefix() const {
    return 1;
}

bool TextDataEntry::compare_unit_desired_siprefix(const DataEngineDataEntry *from) const {
    auto const from_text = dynamic_cast<const TextDataEntry *>(from);
    if (from_text == nullptr) {
        return false;
    }
    if (from_text->desired_value != desired_value) {
        return false;
    }
    return true;
}

void TextDataEntry::set_actual_value(QString actual_value) {
    this->actual_value = actual_value;
}

EntryType TextDataEntry::get_entry_type() const {
    return EntryType::String;
}

void TextDataEntry::set_desired_value_from_desired(DataEngineDataEntry *from) {
    TextDataEntry *num_from = from->as<TextDataEntry>();
    assert(num_from);
    desired_value = num_from->desired_value;
}

void TextDataEntry::set_desired_value_from_actual(DataEngineDataEntry *from) {
    TextDataEntry *num_from = from->as<TextDataEntry>();
    assert(num_from);
    desired_value = num_from->actual_value;
}

bool TextDataEntry::is_desired_value_set() const {
    return (bool)desired_value;
}

QJsonObject TextDataEntry::get_specific_json_dump() const {
    QJsonObject result;
    QJsonObject desired;
    QJsonObject actual;
    desired["desired_is_set"] = (bool)desired_value;
    if ((bool)desired_value) {
        desired["as_text"] = get_desired_value_as_string();
        desired["value"] = desired_value.value();
    }

    actual["actual_is_set"] = (bool)actual_value;
    if ((bool)actual_value) {
        actual["as_text"] = get_desired_value_as_string();
        actual["value"] = actual_value.value();
    }

    result["actual"] = actual;
    result["desired"] = desired;
    return result;
}

QString TextDataEntry::get_specific_json_name() const {
    return "text";
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

BoolDataEntry::BoolDataEntry(const BoolDataEntry &other)
    : DataEngineDataEntry{other.field_name}
    , desired_value{other.desired_value}
    , description{other.description}
    , actual_value{other.actual_value} {}

BoolDataEntry::BoolDataEntry(const FormID name, std::experimental::optional<bool> desired_value, QString description)
    : DataEngineDataEntry(name)
    , desired_value(std::move(desired_value))
    , description(description) {}

bool BoolDataEntry::is_complete() const {
    if ((bool)actual_value == false) {
        return false;
    }
    return true;
}

bool BoolDataEntry::is_in_range() const {
    if (is_complete() == false) {
        return false;
    }
    if ((bool)desired_value) {
        if (actual_value.value() != desired_value) {
            return false;
        }
    }
    return true;
}

QString BoolDataEntry::get_actual_values() const {
    if ((bool)actual_value == false) {
        return unavailable_value;
    } else if (actual_value.value()) {
        return "Yes";
    } else {
        return "No";
    }
}

double BoolDataEntry::get_actual_number() const {
    throw DataEngineError(DataEngineErrorNumber::actual_value_is_not_a_number, QString("Dataengine: Actual value of field %1 is not a number").arg(field_name));
    return 0;
}

QString BoolDataEntry::get_description() const {
    return description;
}

QString BoolDataEntry::get_desired_value_as_string() const {
    if ((bool)desired_value) {
        if (desired_value.value()) {
            return "Yes";
        } else {
            return "No";
        }
    } else {
        return "";
    }
}

QString BoolDataEntry::get_unit() const {
    return "";
}

double BoolDataEntry::get_si_prefix() const {
    return 1;
}

bool BoolDataEntry::compare_unit_desired_siprefix(const DataEngineDataEntry *from) const {
    auto const from_bool = dynamic_cast<const BoolDataEntry *>(from);
    if (from_bool == nullptr) {
        return false;
    }
    if (from_bool->desired_value != desired_value) {
        return false;
    }
    return true;
}

void BoolDataEntry::set_actual_value(bool value) {
    this->actual_value = value;
}

EntryType BoolDataEntry::get_entry_type() const {
    return EntryType::Bool;
}

void BoolDataEntry::set_desired_value_from_desired(DataEngineDataEntry *from) {
    BoolDataEntry *num_from = from->as<BoolDataEntry>();
    assert(num_from);
    desired_value = num_from->desired_value;
}

void BoolDataEntry::set_desired_value_from_actual(DataEngineDataEntry *from) {
    BoolDataEntry *bool_from = from->as<BoolDataEntry>();
    assert(bool_from);
    desired_value = bool_from->actual_value;
}

bool BoolDataEntry::is_desired_value_set() const {
    return (bool)desired_value;
}

QJsonObject BoolDataEntry::get_specific_json_dump() const {
    QJsonObject result;
    QJsonObject desired;
    QJsonObject actual;
    desired["desired_is_set"] = (bool)desired_value;
    if ((bool)desired_value) {
        desired["as_text"] = get_desired_value_as_string();
        desired["value"] = desired_value.value();
    }

    actual["actual_is_set"] = (bool)actual_value;
    if ((bool)actual_value) {
        actual["as_text"] = get_desired_value_as_string();
        actual["value"] = actual_value.value();
    }

    result["actual"] = actual;
    result["desired"] = desired;
    return result;
}

QString BoolDataEntry::get_specific_json_name() const {
    return "bool";
}

ReferenceDataEntry::ReferenceDataEntry(const ReferenceDataEntry &other)
    : DataEngineDataEntry{other.field_name}
    , tolerance{other.tolerance}
    , description{other.description}
    , reference_links{other.reference_links}
    , entry_target{other.entry_target} {
    auto entry_num = dynamic_cast<NumericDataEntry *>(other.entry.get());
    auto entry_bool = dynamic_cast<BoolDataEntry *>(other.entry.get());
    auto entry_text = dynamic_cast<TextDataEntry *>(other.entry.get());
    if (entry_num) {
        entry = std::make_unique<NumericDataEntry>(*entry_num);
    } else if (entry_bool) {
        entry = std::make_unique<BoolDataEntry>(*entry_bool);
    } else if (entry_text) {
        entry = std::make_unique<TextDataEntry>(*entry_text);
    } else if (other.entry.get() == nullptr) {
        //not yet initialized. Is ok
    } else {
        assert(0);
        //unknown data type
        //reference_type would not be allowed since ther is no recursion(reference pointing to a reference)
    }
}

ReferenceDataEntry::ReferenceDataEntry(const FormID name, QString reference_string, NumericTolerance tolerance, QString description)
    : DataEngineDataEntry(name)
    , tolerance(tolerance)
    , description(description) {
    parse_refence_string(reference_string);
}

bool ReferenceDataEntry::is_complete() const {
    if (not_defined_yet_due_to_undefined_instance_count) {
        return false;
    }
    update_desired_value_from_reference();
    if (!entry->is_desired_value_set()) {
        return false;
    }
    return entry->is_complete();
}

bool ReferenceDataEntry::is_in_range() const {
    if (not_defined_yet_due_to_undefined_instance_count) {
        return false;
    }
    update_desired_value_from_reference();
    if (!entry->is_desired_value_set()) {
        return false;
    }
    return entry->is_in_range();
}

void ReferenceDataEntry::update_desired_value_from_reference() const {
    assert_that_instance_count_is_defined();
    if (reference_links[0].value == ReferenceLink::ReferenceValue::DesiredValue) {
        assert(entry_target->is_desired_value_set());
        entry->set_desired_value_from_desired(entry_target);
    } else {
        if ((bool)target_instance_count == false) {
            //assert(0);
            throw DataEngineError(
                DataEngineErrorNumber::reference_must_not_point_to_undefined_instance,
                QString("Dataengine: References must not point to the actual value of an undefined instance. The referencing fieldname: \"%1\"")
                    .arg(field_name));
        }
        if (target_instance_count.value() != 1) {
            throw DataEngineError(
                DataEngineErrorNumber::reference_must_not_point_to_multiinstance_actual_value,
                QString("Dataengine: References must not point to the actual value of multi-instance-fields. The referencing fieldname: \"%1\", the "
                        "reference target is: \"%2\"")
                    .arg(field_name)
                    .arg(entry_target->field_name));
        }
        entry->set_desired_value_from_actual(entry_target);
    }
}

void ReferenceDataEntry::set_actual_value(double number) {
    assert_that_instance_count_is_defined();
    NumericDataEntry *num_entry = entry->as<NumericDataEntry>();
    if (!num_entry) {
        throw DataEngineError(DataEngineErrorNumber::setting_reference_actual_value_with_wrong_type,
                              QString("Dataengine: The referencing field \"%1\" is not a number as it must be if you set it with the number: \"%2\"")
                                  .arg(field_name)
                                  .arg(number));
    }

    num_entry->set_actual_value(number);
}

void ReferenceDataEntry::set_actual_value(QString val) {
    assert_that_instance_count_is_defined();
    TextDataEntry *text_entry = entry->as<TextDataEntry>();
    if (!text_entry) {
        throw DataEngineError(
            DataEngineErrorNumber::setting_reference_actual_value_with_wrong_type,
            QString("Dataengine: The referencing field \"%1\" is not a string as it must be if you set it with the string: \"%2\"").arg(field_name).arg(val));
    }
    text_entry->set_actual_value(val);
}

void ReferenceDataEntry::set_actual_value(bool val) {
    assert_that_instance_count_is_defined();
    BoolDataEntry *bool_entry = entry->as<BoolDataEntry>();
    if (!bool_entry) {
        throw DataEngineError(
            DataEngineErrorNumber::setting_reference_actual_value_with_wrong_type,
            QString("Dataengine: The referencing field \"%1\" is not a bool as it must be if you set it with the bool: \"%2\"").arg(field_name).arg(val));
    }
    bool_entry->set_actual_value(val);
}

QString ReferenceDataEntry::get_actual_values() const {
    assert_that_instance_count_is_defined();
    return entry->get_actual_values();
}

double ReferenceDataEntry::get_actual_number() const {
    assert_that_instance_count_is_defined();
    return entry->get_actual_number();
}

QString ReferenceDataEntry::get_description() const {
    assert_that_instance_count_is_defined();
    return description;
}

QString ReferenceDataEntry::get_desired_value_as_string() const {
    assert_that_instance_count_is_defined();
    update_desired_value_from_reference();
    return entry->get_desired_value_as_string();
}

QString ReferenceDataEntry::get_unit() const {
    assert_that_instance_count_is_defined();
    assert(entry_target);
    return entry_target->get_unit();
}

double ReferenceDataEntry::get_si_prefix() const {
    assert_that_instance_count_is_defined();
    assert(entry_target);
    return entry_target->get_si_prefix();
}

EntryType ReferenceDataEntry::get_entry_type() const {
    assert_that_instance_count_is_defined();
    assert(entry_target);
    return entry_target->get_entry_type();
}

bool ReferenceDataEntry::compare_unit_desired_siprefix(const DataEngineDataEntry *from) const {
    (void)from;
    assert(0);
    return false;
}

void ReferenceDataEntry::dereference(DataEngineSections *sections) {
    uint i = 0;
    not_defined_yet_due_to_undefined_instance_count = false;
    while (i < reference_links.size()) {
        auto section = sections->get_section(reference_links[i].link);
        if (!section->is_section_instance_defined()) {
            not_defined_yet_due_to_undefined_instance_count = true;
            break;
        }

        if (sections->exists_uniquely(reference_links[i].link)) {
            i++;
        } else {
            reference_links.erase(reference_links.begin() + i);
        }
    }
    if (not_defined_yet_due_to_undefined_instance_count) {
        return;
    }

    if (reference_links.size() == 0) {
        throw DataEngineError(DataEngineErrorNumber::reference_not_found, QString("Dataengine: reference non existing. Fieldname: \"%1\"").arg(field_name));

    } else if (reference_links.size() > 1) {
        QString t;
        for (auto &ref : reference_links) {
            t += ref.link + " ";
        }
        throw DataEngineError(DataEngineErrorNumber::reference_ambiguous,
                              QString("Dataengine: reference ambiguous  with links: \"%1\", fieldname: \"%2\"").arg(t).arg(field_name));
    }
    auto targets = sections->get_entries_const(reference_links[0].link);

    target_instance_count = targets.count();
    if (have_entries_equal_desired_values(targets)) {
        entry_target = const_cast<DataEngineDataEntry *>(targets[0]);
    } else {
        throw DataEngineError(DataEngineErrorNumber::reference_pointing_to_multiinstance_with_different_values,
                              QString("Dataengine: Reference \"%1\" points to multiinstance with different values. This is not allowed.").arg(field_name));
    }

    if (reference_links[0].value == ReferenceLink::ReferenceValue::DesiredValue) {
        if (!entry_target->is_desired_value_set()) {
            throw DataEngineError(DataEngineErrorNumber::reference_target_has_no_desired_value,
                                  QString("Dataengine: reference \"%1\" points to desired value \"%2\", even though no desired value is defined in \"%2\".")
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
                QString("Dataengine: reference \"%1\" pointing to \"%2\", is a number but has no tolerance defined. A tolerance must be defined on numbers.")
                    .arg(field_name)
                    .arg(reference_links[0].link));
        }
        entry = std::make_unique<NumericDataEntry>("", temp_desired_value, tolerance, num_entry->unit, num_entry->get_si_prefix(), description);
    } else if (text_entry) {
        std::experimental::optional<QString> temp_desired_value; //will be set later, when beeing compared
        entry = std::make_unique<TextDataEntry>("", temp_desired_value, description);
    } else if (bool_entry) {
        std::experimental::optional<bool> temp_desired_value; //will be set later, when beeing compared
        entry = std::make_unique<BoolDataEntry>("", temp_desired_value, description);
    } else {
        assert(0); //TODO: throw illegal type
    }
    if (!num_entry) {
        if (tolerance.is_defined()) {
            throw DataEngineError(
                DataEngineErrorNumber::reference_is_not_number_but_has_tolerance,
                QString("Dataengine: reference \"%1\" pointing to \"%2\", is not a number but has a tolerance defined. A tolerance is only allowed "
                        "to be applied on numbers.")
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
            throw DataEngineError(
                DataEngineErrorNumber::illegal_reference_declaration,
                QString("Dataengine: reference \"%1\" target declaration must end with \".desired\" or \".actual\" but does not.").arg(field_name));
        }
        reference_links.push_back(ref_link);
    }
}

void ReferenceDataEntry::assert_that_instance_count_is_defined() const {
    if (not_defined_yet_due_to_undefined_instance_count) {
        throw DataEngineError(DataEngineErrorNumber::reference_cant_be_used_because_its_pointing_to_a_yet_undefined_instance,
                              QString("Dataengine: reference \"%1\" can not be used since it is pointing to an instance which is undefinded").arg(field_name));
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

bool ReferenceDataEntry::is_desired_value_set() const {
    assert_that_instance_count_is_defined();
    assert(entry_target);
    return entry_target->is_desired_value_set();
}

QJsonObject ReferenceDataEntry::get_specific_json_dump() const {
    assert_that_instance_count_is_defined();
    assert(entry_target);
    return entry_target->get_specific_json_dump();
}

QString ReferenceDataEntry::get_specific_json_name() const {
    assert_that_instance_count_is_defined();
    assert(entry_target);
    return entry_target->get_specific_json_name();
}

DataEngineActualValueStatisticFile::DataEngineActualValueStatisticFile() {
    is_opened = false;
}

DataEngineActualValueStatisticFile::~DataEngineActualValueStatisticFile() {
 //   close_file();
}

void DataEngineActualValueStatisticFile::start_recording(QString file_root_path, QString file_prefix) {
    this->file_root_path = file_root_path;
    this->file_prefix = file_prefix;
    open_or_create_new_file();
}

QString select_newest_file_name(QStringList file_list, QString prefix) {
    QDateTime dt_max = QDateTime::fromMSecsSinceEpoch(0);
    QString max_file_name{""};
    for (const QString &file_name : file_list) {
        QDateTime dt = decode_date_time_from_file_name(file_name.toStdString(), prefix.toStdString());
        //  qDebug() << dt.toString();
        if (dt > dt_max) {
            max_file_name = file_name;
            dt_max = dt;
        }
    }
    return max_file_name;
}

QString DataEngineActualValueStatisticFile::select_file_name_to_be_used(QStringList file_list) {
    return select_newest_file_name(file_list, file_prefix);
}

void DataEngineActualValueStatisticFile::save_to_file()
{
    close_file();
}

void DataEngineActualValueStatisticFile::open_or_create_new_file() {
    QStringList nameFilter(file_prefix + "*.json");
    QString root_path = append_separator_to_path(file_root_path);
    QDir directory(root_path);
    directory.mkpath(root_path);
    QString file_name = select_file_name_to_be_used(directory.entryList(nameFilter));
    bool use_new_file = true;
    if (file_name.size()) {
        file_name = directory.filePath(file_name);
        use_new_file = false;
        open_file(file_name);
        for (const QString &key : data_entries.keys()) {
            QJsonArray arr = data_entries[key].toArray();
            if ((uint)arr.count() > entry_limit) {
                use_new_file = true;
                break;
            }
        }
    }
    if (use_new_file) {
        create_new_file();
    }
}

void DataEngineActualValueStatisticFile::create_new_file() {
    QString file_name = QString::fromStdString(propose_unique_filename_by_datetime(file_root_path.toStdString(), file_prefix.toStdString(), ".json"));
    open_file(file_name);
}

void DataEngineActualValueStatisticFile::close_file() {
    if (is_opened && used_file_name.size()) {
        QFile saveFile(used_file_name);
        if (!saveFile.open(QIODevice::WriteOnly)) {
            throw DataEngineError(DataEngineErrorNumber::cannot_open_file,
                                  QString{"Dataengine: Can not open file: \"%1\" for saving actual value statistics."}.arg(used_file_name));
            remove_lock_file();
            return;
        }
        QJsonDocument saveDoc(data_entries);
        saveFile.write(saveDoc.toJson());
        data_entries = QJsonObject{};
        remove_lock_file();
        is_opened = false;
    }
}

void DataEngineActualValueStatisticFile::remove_lock_file() {
    if (lock_file_exists) {
        QString lock_file_name = used_file_name + ".lock";
        QFile file(lock_file_name);
        file.remove();
        lock_file_exists = false;
    }
}

bool DataEngineActualValueStatisticFile::check_and_create_lock_file() {
    bool result = false;
    QString lock_file_name = used_file_name + ".lock";
    if (QFile::exists(lock_file_name)) {
        result = true;
    }

    QFile lockfile(lock_file_name);
    lockfile.open(QIODevice::WriteOnly);
    lock_file_exists = true;
    return result;
}

void DataEngineActualValueStatisticFile::open_file(QString file_name) {
    qDebug() << file_name;
    close_file();
    used_file_name = file_name;
    if (check_and_create_lock_file()) {
        used_file_name = QString::fromStdString(propose_unique_filename_by_datetime(file_root_path.toStdString(), file_prefix.toStdString(), ".json"));
    }

    QFile loadFile(used_file_name);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        data_entries = QJsonObject{};
    } else {
        QByteArray byte_data = loadFile.readAll();
        QJsonDocument loadDoc(QJsonDocument::fromJson(byte_data));
        data_entries = loadDoc.object();
    }
    is_opened = true;
}

void DataEngineActualValueStatisticFile::set_actual_value(const FormID &field_name, const QString serialised_dependency, double value) {
    if (is_opened) {
        QJsonObject obj{};
        obj["time_stamp"] = QDateTime::currentMSecsSinceEpoch();
        obj["value"] = value;
        if (dut_identifier != ""){
            obj["dut_id"] = dut_identifier;
        }
        QJsonArray arr;
        QString key_name = field_name;
        if (serialised_dependency.size()) {
            key_name = key_name + "~" + serialised_dependency;
        }
        if (data_entries[key_name].isArray()) {
            arr = data_entries[key_name].toArray();
        }
        arr.append(obj);
        data_entries[key_name] = arr;
        if ((uint)arr.count() > entry_limit) {
            create_new_file();
        }
    }
}

void DataEngineActualValueStatisticFile::set_dut_identifier(QString dut_identifier)
{
    this->dut_identifier = dut_identifier;
}
