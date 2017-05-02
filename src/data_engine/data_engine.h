#ifndef DATA_ENGINE_H
#define DATA_ENGINE_H

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
class QtRPT;
class DataEngineSections;

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
    illegal_reference_declaration,
    setting_reference_actual_value_with_wrong_type,
    instance_count_must_not_be_zero,
    instance_count_does_not_exist,
    instance_count_yet_undefined,
    instance_count_already_defined,
    instance_count_exceeding
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
    virtual QStringList get_actual_values() const = 0;
    virtual QString get_description() const = 0;
    virtual QString get_desired_value_as_string() const = 0;
    virtual QString get_unit() const = 0;

    virtual void set_desired_value_from_desired(DataEngineDataEntry *from) = 0;
    virtual void set_desired_value_from_actual(DataEngineDataEntry *from) = 0;
    virtual bool is_desired_value_set() = 0;

    virtual void set_instance_count(uint instance_count) = 0;
    virtual void set_actual_instance_index(uint instance_index) = 0;

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
    NumericDataEntry(FormID field_name, std::experimental::optional<double> desired_value, NumericTolerance tolerance, QString unit,
                     std::experimental::optional<double> si_prefix, QString description);
    bool valid() const;
    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_desired_value_as_string() const override;
    QStringList get_actual_values() const override;
    QString get_description() const override;
    QString get_unit() const override;

    void set_actual_value(std::experimental::optional<double> actual_value);
    std::experimental::optional<double> desired_value{};
    QString unit{};
    QString description{};

    double si_prefix = 1.0;
    void set_instance_count(uint instance_count) override;
    void set_actual_instance_index(uint instance_index) override;

    private:
    NumericTolerance tolerance;
    void set_desired_value_from_desired(DataEngineDataEntry *from) override;
    void set_desired_value_from_actual(DataEngineDataEntry *from) override;
    bool is_desired_value_set() override;
    std::vector<std::experimental::optional<double>> actual_values{};
    uint actual_instance_index = 0;
};

struct TextDataEntry : DataEngineDataEntry {
    TextDataEntry(const FormID name, std::experimental::optional<QString> desired_value, QString description);

    bool is_complete() const override;
    bool is_in_range() const override;
    QStringList get_actual_values() const override;
    QString get_description() const override;
    QString get_desired_value_as_string() const override;
    QString get_unit() const override;

    std::experimental::optional<QString> desired_value{};
    QString description{};

    void set_actual_value(QString actual_value);
    void set_instance_count(uint instance_count) override;
    void set_actual_instance_index(uint instance_index) override;

    private:
    void set_desired_value_from_desired(DataEngineDataEntry *from) override;
    void set_desired_value_from_actual(DataEngineDataEntry *from) override;
    bool is_desired_value_set() override;
    std::vector<std::experimental::optional<QString>> actual_values{};
    uint actual_instance_index;
};

struct BoolDataEntry : DataEngineDataEntry {
    BoolDataEntry(const FormID name, std::experimental::optional<bool> desired_value, QString description);

    bool is_complete() const override;
    bool is_in_range() const override;
    QStringList get_actual_values() const override;
    QString get_description() const override;
    QString get_desired_value_as_string() const override;
    QString get_unit() const override;

    std::experimental::optional<bool> desired_value{};
    QString description{};

    void set_actual_value(bool value);
    void set_instance_count(uint instance_count) override;
    void set_actual_instance_index(uint instance_index) override;

    private:
    void set_desired_value_from_desired(DataEngineDataEntry *from) override;
    void set_desired_value_from_actual(DataEngineDataEntry *from) override;
    bool is_desired_value_set() override;
    std::vector<std::experimental::optional<bool>> actual_values{};
    uint actual_instance_index = 0;
};

struct ReferenceLink {
    enum class ReferenceValue { ActualValue, DesiredValue };
    FormID link;
    ReferenceValue value;
};

struct ReferenceDataEntry : DataEngineDataEntry {
    ReferenceDataEntry(const FormID name, QString reference_string, NumericTolerance tolerance, QString description);

    //referenz erbt typ vom target
    //referenz erbt unit vom target
    //referenz si-prefix unit vom target
    //referenz erbt sollwert von target, dies kann jeweils enweder soll oder ist wert sein.
    bool is_complete() const override;
    bool is_in_range() const override;
    QStringList get_actual_values() const override;
    QString get_description() const override;
    QString get_desired_value_as_string() const override;
    QString get_unit() const override;
    NumericTolerance tolerance;
    QString description{};
    void dereference(DataEngineSections *sections);

    void set_actual_value(double number);
    void set_actual_value(QString val);
    void set_actual_value(bool val);

    void set_instance_count(uint instance_count) override;
    void set_actual_instance_index(uint instance_index) override;

    private:
    void parse_refence_string(QString reference_string);
    std::vector<ReferenceLink> reference_links;

    DataEngineDataEntry *entry_target;

    std::unique_ptr<DataEngineDataEntry> entry;

    private:
    void set_desired_value_from_desired(DataEngineDataEntry *from) override;
    void set_desired_value_from_actual(DataEngineDataEntry *from) override;
    bool is_desired_value_set() override;
    void update_desired_value_from_reference() const;
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
    DependencyTags dependency_tags;
    std::vector<std::unique_ptr<DataEngineDataEntry>> data_entries;

    public:
    bool is_dependency_matching(const QMap<QString, QVariant> &tags) const;
    void from_json(const QJsonObject &object);
    bool value_exists(QString field_name);
    void set_instance_count(uint instance_count);
    void set_actual_instance_index(uint instance_index);
    DataEngineDataEntry *get_value(QString field_name) const;
};

struct DataEngineSection {
    std::vector<VariantData> variants;

    public:
    const VariantData *get_variant() const;
    void delete_unmatched_variants(const QMap<QString, QVariant> &tags);
    bool is_complete() const;
    bool all_values_in_range() const;

    void from_json(const QJsonValue &object, const QString &key_name);
    QString get_section_name() const;
    QString get_instance_count_name() const;

    bool is_section_instance_defined() const;

    bool set_instance_count_if_name_matches(QString instance_count_name, uint instance_count);
    void use_instance(QString instance_caption, uint instance_index);
    void set_instance_index(uint instance_index);
    void set_actual_instance_caption(QString instance_caption);
    void create_instances_if_defined();
    QStringList instance_captions;
    private:
    std::experimental::optional<uint> instance_count;
    QString instance_count_name;
    QString section_name;
    void append_variant_from_json(const QJsonObject &object);
};

struct DataEngineSections {
    std::vector<DataEngineSection> sections;
    void delete_unmatched_variants(const QMap<QString, QVariant> &tags);

    public:
    const DataEngineDataEntry *get_entry(const FormID &id) const;
    DataEngineDataEntry *get_entry(const FormID &id);
    bool exists_uniquely(const FormID &id) const;
    bool is_complete() const;
    bool all_values_in_range() const;
    void from_json(const QJsonObject &object);
    bool section_exists(QString section_name);
    void deref_references();
    void set_instance_count(QString instance_count_name, uint instance_count);
    void use_instance(QString section_name, QString instance_caption, uint instance_index);
    void create_already_defined_instances();

    private:
    const DataEngineDataEntry *get_entry_raw(const FormID &id, DataEngineErrorNumber *error_num, QString &section_name, QString &field_name) const;
    DataEngineSection *get_section_raw(const QString &section_name, DataEngineErrorNumber *error_num) const;
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
    Data_engine(std::istream &source, const QMap<QString, QVariant> &tags);

    void set_source(std::istream &source, const QMap<QString, QVariant> &tags);
    bool is_complete() const;
    bool all_values_in_range() const;
    bool value_in_range(const FormID &id) const;
    void set_actual_number(const FormID &id, double number);
    void set_actual_text(const FormID &id, QString text);
    void set_actual_bool(const FormID &id, bool value);
    void use_instance(const QString &section_name, const QString &instance_caption, const uint instance_index);
    void set_instance_count(QString instance_count_name, uint instance_count);

    // double get_desired_value(const FormID &id) const;

    QStringList get_actual_values(const FormID &id) const;
    QString get_description(const FormID &id) const;
    QString get_desired_value_as_string(const FormID &id) const;
    QString get_unit(const FormID &id) const;

    Statistics get_statistics() const;

    std::unique_ptr<QWidget> get_preview() const;
    void generate_pdf(const std::string &form, const std::__cxx11::string &destination) const;
    std::string get_json() const;

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

    static bool entry_compare(const FormIdWrapper &lhs, const FormIdWrapper &rhs);

    DataEngineSections sections;

    void setValue(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage) const;
    void fill_report(QtRPT &report, const QString &form) const;
};

#endif // DATA_ENGINE_H
