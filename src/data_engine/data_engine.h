#ifndef DATA_ENGINE_H
#define DATA_ENGINE_H
#include "sol.hpp"

#include <QList>
#include <QString>
#include <experimental/optional>
#include <istream>
#include <memory>
#include <string>
#include <vector>

class QJsonObject;
class QWidget;
class QVariant;
class DataEngineSections;
class QSqlDatabase;

using FormID = QString;
enum class EntryType { Unspecified, Bool, String, Reference, Numeric };

enum class DataEngineErrorNumber {
    ok,
    invalid_version_dependency_string,
    no_data_section_found,
    invalid_data_entry_key,
    invalid_data_entry_type,
    invalid_json_object,
    data_entry_contains_no_name,
    data_entry_contains_neither_type_nor_value,
    tolerance_parsing_error,
    tolerance_must_be_defined_for_range_checks_on_numbers,
    tolerance_must_be_defined_for_numbers,
    duplicate_field,
    duplicate_section,
    non_unique_desired_field_found,
    no_variant_found,
    no_section_id_found,
    no_field_id_found,
    faulty_field_id,
    setting_desired_value_with_wrong_type,
    reference_ambiguous,
    reference_not_found,
    reference_target_has_no_desired_value,
    reference_is_not_number_but_has_tolerance,
    reference_is_a_number_and_needs_tolerance,
    reference_must_not_point_to_multiinstance_actual_value,
    reference_pointing_to_multiinstance_with_different_values,
    illegal_reference_declaration,
    setting_reference_actual_value_with_wrong_type,
    wrong_type_for_instance_count,
    instance_count_must_not_be_zero_nor_fraction_nor_negative,
    instance_count_does_not_exist,
    instance_count_yet_undefined,
    instance_count_already_defined,
    instance_count_exceeding,
    instance_count_must_not_be_defined_in_variant_scope,
    allow_empty_section_must_not_be_defined_in_variant_scope,
    allow_empty_section_with_wrong_type,
    instance_count_must_match_list_of_dependency_values,
    list_of_dependency_values_must_be_of_equal_length,
    reference_cant_be_used_because_its_pointing_to_a_yet_undefined_instance,
	is_in_dummy_mode,
	sql_error,
};
class DataEngineError : public std::runtime_error {
    public:
    DataEngineError(DataEngineErrorNumber err_number, const QString &str)
        : std::runtime_error(str.toStdString())
        , error_number(err_number) {}

    DataEngineErrorNumber get_error_number() const {
        return error_number;
    }

    private:
    DataEngineErrorNumber error_number;
};

struct DataEngineDataEntry {
    DataEngineDataEntry(const FormID &field_name)
        : field_name(field_name) {}
    FormID field_name;

    virtual bool is_complete() const = 0;
    virtual bool is_in_range() const = 0;
    virtual QString get_actual_values() const = 0;
    virtual QString get_description() const = 0;
    virtual QString get_desired_value_as_string() const = 0;
    virtual QString get_unit() const = 0;
    virtual double get_si_prefix() const = 0;
    virtual void set_desired_value_from_desired(DataEngineDataEntry *from) = 0;
    virtual void set_desired_value_from_actual(DataEngineDataEntry *from) = 0;
    virtual bool is_desired_value_set() const = 0;
    virtual bool compare_unit_desired_siprefix(const DataEngineDataEntry *from) const = 0;
    virtual EntryType get_entry_type() const = 0;

    template <class T>
    T *as();
    template <class T>
    const T *as() const;
    static std::unique_ptr<DataEngineDataEntry> from_json(const QJsonObject &object);
    virtual ~DataEngineDataEntry() = default;
};

template <class T>
T *DataEngineDataEntry::DataEngineDataEntry::as() {
    return dynamic_cast<T *>(this);
}

template <class T>
const T *DataEngineDataEntry::DataEngineDataEntry::as() const {
    return dynamic_cast<const T *>(this);
}

struct NumericTolerance {
    enum ToleranceType { Absolute, Percent };

    bool test_in_range(const double desired, const std::experimental::optional<double> &measured) const;

    public:
    void from_string(const QString &str);
    QString to_string(const double desired_value) const;
    bool is_defined() const;

    private:
    bool is_undefined = true;
    void str_to_num(QString str_in, double &number, ToleranceType &tol_type, bool &open, QStringList expected_sign_strings);
    QString num_to_str(double number, ToleranceType tol_type) const;

    ToleranceType tolerance_type;
    double deviation_limit_above;
    bool open_range_above;

    double deviation_limit_beneath;
    bool open_range_beneath;
};

struct NumericDataEntry : DataEngineDataEntry {
    NumericDataEntry(const NumericDataEntry &other);

    NumericDataEntry(FormID field_name, std::experimental::optional<double> desired_value, NumericTolerance tolerance, QString unit,
                     std::experimental::optional<double> si_prefix, QString description);

    bool valid() const;
    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_desired_value_as_string() const override;
    QString get_actual_values() const override;
    QString get_description() const override;
    QString get_unit() const override;
    double get_si_prefix() const override;
    bool compare_unit_desired_siprefix(const DataEngineDataEntry *from) const override;
    void set_actual_value(double actual_value);
    EntryType get_entry_type() const override;
    bool is_desired_value_set() const override;

    std::experimental::optional<double> desired_value{};
    QString unit{};
    QString description{};

    private:
    void set_desired_value_from_desired(DataEngineDataEntry *from) override;
    void set_desired_value_from_actual(DataEngineDataEntry *from) override;

    double si_prefix = 1.0;
    NumericTolerance tolerance;
    std::experimental::optional<double> actual_value;
};

struct TextDataEntry : DataEngineDataEntry {
    TextDataEntry(const TextDataEntry &other);

    TextDataEntry(const FormID name, std::experimental::optional<QString> desired_value, QString description);

    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_actual_values() const override;
    QString get_description() const override;
    QString get_desired_value_as_string() const override;
    QString get_unit() const override;
    double get_si_prefix() const override;
    bool compare_unit_desired_siprefix(const DataEngineDataEntry *from) const override;
    void set_actual_value(QString actual_value);
    EntryType get_entry_type() const override;
    bool is_desired_value_set() const override;

    std::experimental::optional<QString> desired_value{};
    QString description{};

    private:
    void set_desired_value_from_desired(DataEngineDataEntry *from) override;
    void set_desired_value_from_actual(DataEngineDataEntry *from) override;

    std::experimental::optional<QString> actual_value{};
};

struct BoolDataEntry : DataEngineDataEntry {
    BoolDataEntry(const BoolDataEntry &other);
    BoolDataEntry(const FormID name, std::experimental::optional<bool> desired_value, QString description);

    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_actual_values() const override;
    QString get_description() const override;
    QString get_desired_value_as_string() const override;
    QString get_unit() const override;
    double get_si_prefix() const override;
    bool compare_unit_desired_siprefix(const DataEngineDataEntry *from) const override;
    void set_actual_value(bool value);
    EntryType get_entry_type() const override;
    bool is_desired_value_set() const override;

    std::experimental::optional<bool> desired_value{};
    QString description{};

    private:
    void set_desired_value_from_desired(DataEngineDataEntry *from) override;
    void set_desired_value_from_actual(DataEngineDataEntry *from) override;

    std::experimental::optional<bool> actual_value{};
};

struct ReferenceLink {
    enum class ReferenceValue { ActualValue, DesiredValue };
    FormID link;
    ReferenceValue value;
};

struct ReferenceDataEntry : DataEngineDataEntry {
    ReferenceDataEntry(const ReferenceDataEntry &other);
    ReferenceDataEntry(const FormID name, QString reference_string, NumericTolerance tolerance, QString description);

    //referenz erbt typ vom target
    //referenz erbt unit vom target
    //referenz si-prefix unit vom target
    //referenz erbt sollwert von target, dies kann jeweils enweder soll oder ist wert sein.
    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_actual_values() const override;
    QString get_description() const override;
    QString get_desired_value_as_string() const override;
    QString get_unit() const override;
    double get_si_prefix() const override;
    EntryType get_entry_type() const override;
    bool compare_unit_desired_siprefix(const DataEngineDataEntry *from) const override;
    void set_actual_value(double number);
    void set_actual_value(QString val);
    void set_actual_value(bool val);
    void dereference(DataEngineSections *sections);
    bool is_desired_value_set() const override;

    NumericTolerance tolerance;
    QString description{};

    private:
    void set_desired_value_from_desired(DataEngineDataEntry *from) override;
    void set_desired_value_from_actual(DataEngineDataEntry *from) override;
    void update_desired_value_from_reference() const;
    void parse_refence_string(QString reference_string);
    bool not_defined_yet_due_to_undefined_instance_count = false;
    void assert_that_instance_count_is_defined() const;

    std::vector<ReferenceLink> reference_links;
    DataEngineDataEntry *entry_target;
    std::experimental::optional<uint> target_instance_count;
    std::unique_ptr<DataEngineDataEntry> entry;
};

struct DependencyValue {
    DependencyValue();
    enum Match_style { MatchExactly, MatchByRange, MatchEverything, MatchNone };
    QVariant match_exactly;
    double range_low_including;
    double range_high_excluding;
    Match_style match_style;

    public:
    void from_json(const QJsonValue &object, const bool default_to_match_all);
    bool is_matching(const QVariant &test_value) const;
    void from_string(const QString &str);

    private:
    void from_number(const double &number);
    void from_bool(const bool &boolean);
    void parse_number(const QString &str, float &vnumber, bool &matcheverything);
};

struct DependencyTags {
    QMultiMap<QString, DependencyValue> tags;

    public:
    void from_json(const QJsonValue &object);
};

struct VariantData {
    VariantData();
    VariantData(const VariantData &other);

    //VariantData(const VariantData &) = delete;
    VariantData &operator=(const VariantData &) = delete;
    ~VariantData() = default;

    DependencyTags dependency_tags;
    std::vector<std::unique_ptr<DataEngineDataEntry>> data_entries;
    uint get_entry_count() const;

    public:
    bool is_dependency_matching(const QMap<QString, QList<QVariant>> &tags, uint instance_index, uint instance_count, const QString &section_name) const;
    void from_json(const QJsonObject &object);
    bool entry_exists(QString field_name);
    DataEngineDataEntry *get_entry(QString field_name) const;
    DataEngineDataEntry *get_entry_raw(QString field_name, DataEngineErrorNumber *errornum) const;
};

struct DataEngineInstance {
    DataEngineInstance();
    DataEngineInstance(const DataEngineInstance &other);
    DataEngineInstance(DataEngineInstance &&other); //move constructor

    void delete_unmatched_variants(const QMap<QString, QList<QVariant>> &tags, uint instance_index, uint instance_count);
    void delete_all_but_biggest_variants();
    void set_section_name(QString section_name);
    void set_allow_empty_section(bool allow_empty_section);
    const VariantData *get_variant() const;

    std::vector<VariantData> variants;
    QString instance_caption;

    private:
    QString section_name;
    bool allow_empty_section = false;
};

struct DataEngineSection {
    std::vector<DataEngineInstance> instances;

    public:
    void delete_unmatched_variants(const QMap<QString, QList<QVariant>> &tags);
    void delete_all_but_biggest_variants();
    bool is_complete() const;
    bool all_values_in_range() const;

    void from_json(const QJsonValue &object, const QString &key_name);
    QString get_section_name() const;
    QString get_instance_count_name() const;
    QStringList get_all_ids_of_selected_instance(const QString &prefix) const;
    QStringList get_instance_captions() const;
    bool is_section_instance_defined() const;

    bool set_instance_count_if_name_matches(QString instance_count_name, uint instance_count);
    void use_instance(QString instance_caption, uint instance_index);
    void set_actual_instance_caption(QString instance_caption);
    void create_instances_if_defined();

    void set_actual_number(const FormID &id, double number);
    void set_actual_text(const FormID &id, QString text);
    void set_actual_bool(const FormID &id, bool value);

    DataEngineInstance prototype_instance;

    uint get_actual_instance_index();

    private:
    std::experimental::optional<uint> instance_count;

    QString instance_count_name;
    QString section_name;
    void append_variant_from_json(const QJsonObject &object);
    void assert_instance_is_defined() const;
    uint actual_instance_index = 0;
    DataEngineDataEntry *get_entry(QString id) const;
    const DataEngineInstance *get_actual_instance() const;
};

struct DecodecFieldID {
    QString section_name;
    QString field_name;
};

struct DataEngineSections {
    DataEngineSections();

    std::vector<DataEngineSection> sections;
    void delete_unmatched_variants();

    public:
    QList<const DataEngineDataEntry *> get_entries_const(const FormID &id) const;
    const DataEngineDataEntry *get_actual_instance_entry_const(const FormID &id) const;
    QList<DataEngineDataEntry *> get_entries(const FormID &id);
    bool exists_uniquely(const FormID &id) const;
    bool is_complete() const;
    bool all_values_in_range() const;
    void from_json(const QJsonObject &object);
    bool section_exists(QString section_name);
    void deref_references();
    void set_instance_count(QString instance_count_name, uint instance_count);
    void use_instance(QString section_name, QString instance_caption, uint instance_index);
    void create_already_defined_instances();
    DataEngineSection *get_section(const FormID &id) const;

    static DecodecFieldID decode_field_id(const FormID &id);
    void set_dependancy_tags(const QMap<QString, QList<QVariant>> &tags);
    const QMap<QString, QList<QVariant>> &get_dependancy_tags();
    bool is_dummy_data_mode = true;

    private:
    QList<const DataEngineDataEntry *> get_entries_raw(const FormID &id, DataEngineErrorNumber *error_num, DecodecFieldID &decoded_field_name,
                                                       bool using_instance_index) const;
    DataEngineSection *get_section_raw(const QString &section_name, DataEngineErrorNumber *error_num) const;
    QMap<QString, QList<QVariant>> dependency_tags;
};

class Data_engine {
    struct Statistics {
        int number_of_id_fields{};
        int number_of_data_fields{};
        int number_of_filled_fields{};
        int number_of_inrange_fields{};
        QString to_qstring() const;
    };

    public:
    Data_engine() = default;
    Data_engine(std::istream &source, const QMap<QString, QList<QVariant>> &tags);
    Data_engine(std::istream &source); //for getting dummy data structure
    void set_dependancy_tags(const QMap<QString, QList<QVariant>> &tags);
    void set_source(std::istream &source);
    QStringList get_instance_count_names();

    bool is_complete() const;
    bool all_values_in_range() const;
    bool value_in_range(const FormID &id) const;
    bool value_complete(const FormID &id) const;
    bool is_bool(const FormID &id) const;
    bool is_number(const FormID &id) const;
    bool is_text(const FormID &id) const;
    void set_actual_number(const FormID &id, double number);
    void set_actual_text(const FormID &id, QString text);
    void set_actual_bool(const FormID &id, bool value);
    void use_instance(const QString &section_name, const QString &instance_caption, const uint instance_index);
    void set_instance_count(QString instance_count_name, uint instance_count);

    QString get_actual_value(const FormID &id) const;
    QString get_description(const FormID &id) const;
    QString get_desired_value_as_string(const FormID &id) const;
    QString get_unit(const FormID &id) const;
    EntryType get_entry_type(const FormID &id) const;
    double get_si_prefix(const FormID &id) const;
    bool is_desired_value_set(const FormID &id) const;

	QStringList get_section_names() const;
    sol::table get_section_names(sol::state *lua);
    QStringList get_instance_captions(const QString &section_name) const;
	uint get_instance_count(const std::string &section_name) const;
	sol::table get_ids_of_section(sol::state *lua, const std::string &section_name);
	QStringList get_ids_of_section(const QString &section_name) const;

    void fill_engine_with_dummy_data();
    Statistics get_statistics() const;

    void save_data_to_file(const QString &filename) const;

    std::unique_ptr<QWidget> get_preview() const;
	bool generate_pdf(const std::string &form, const std::string &destination) const;
	void fill_database(QSqlDatabase &db);

    private:
    struct FormIdWrapper {
        FormIdWrapper(const FormID &id)
            : value(id) {}
        FormIdWrapper(const std::unique_ptr<DataEngineDataEntry> &entry)
            : value(entry->get_description()) {}
        FormIdWrapper(const std::pair<FormID, std::unique_ptr<DataEngineDataEntry>> &entry)
            : value(entry.first) {}
        QString value;
    };

    void assert_in_dummy_mode() const;
    static bool entry_compare(const FormIdWrapper &lhs, const FormIdWrapper &rhs);

    DataEngineSections sections;
};

#endif // DATA_ENGINE_H
