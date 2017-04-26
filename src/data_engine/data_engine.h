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

using FormID = QString;
enum class EntryType { Unspecified, Bool, String, Reference, Numeric };

enum class DataEngineErrorNumber {
    invalid_version_dependency_string,
    no_data_section_found,
    invalid_data_entry_key,
    invalid_data_entry_type,
    invalid_json_object,
    data_entry_contains_no_name,
    data_entry_contains_neither_type_nor_value,
    tolerance_parsing_error,
    duplicate_field,
    duplicate_section,
    non_unique_desired_field_found,
    no_section_id_found,
    no_field_id_found,
    faulty_field_id,
    setting_desired_value_with_wrong_type
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
    virtual QString get_value() const = 0;
    virtual QString get_description() const = 0;
    virtual QString get_desired_value_as_string() const = 0;

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

    bool test_in_range(const double desired , const std::experimental::optional<double> &measured) const;

    public:
    void from_string(const QString &str);
    QString to_string(const double desired_value) const;

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
    NumericDataEntry(FormID field_name, std::experimental::optional<double> desired_value, NumericTolerance tolerance, QString unit, QString description);
    bool valid() const;
    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_desired_value_as_string() const override;

    QString get_value() const override;
    QString get_description() const override;

    std::experimental::optional<double> desired_value{};
    QString unit{};
    QString description{};
    std::experimental::optional<double> actual_value{};

    private:
    NumericTolerance tolerance;
};

struct TextDataEntry : DataEngineDataEntry {
    TextDataEntry(const FormID name, std::experimental::optional<QString> desired_value);

    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_value() const override;
    QString get_description() const override;
    QString get_desired_value_as_string() const override;

    std::experimental::optional<QString> desired_value{};
    QString description{};
    std::experimental::optional<QString> actual_value{};
};

struct ReferenceDataEntry : DataEngineDataEntry {
    ReferenceDataEntry(const FormID name, QString reference_string, NumericTolerance tolerance);

    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_value() const override;
    QString get_description() const override;
    QString get_desired_value_as_string() const override;

    QString reference_string;
    NumericTolerance tolerance;
    QString description{};
};

struct BoolDataEntry : DataEngineDataEntry {
    BoolDataEntry(const FormID name, std::experimental::optional<bool> desired_value);

    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_value() const override;
    QString get_description() const override;
    QString get_desired_value_as_string() const override;

    std::experimental::optional<bool> desired_value{};
    QString description{};
    std::experimental::optional<bool> actual_value{};
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
    DataEngineDataEntry *get_value(QString field_name) const;
};

struct DataEngineSection {
    std::vector<VariantData> variants;

    public:
    const VariantData *get_variant(const QMap<QString, QVariant> &tags) const;
    bool is_complete(const QMap<QString, QVariant> &tags) const;
    bool all_values_in_range(const QMap<QString, QVariant> &tags) const;

    void from_json(const QJsonValue &object, const QString &key_name);
    QString get_section_name() const;

    private:
    QString section_name;
    void append_variant_from_json(const QJsonObject &object);
};

struct DataEngineSections {
    std::vector<DataEngineSection> sections;

    public:
    const DataEngineDataEntry *get_entry(const FormID &id, const QMap<QString, QVariant> &tags) const;
    DataEngineDataEntry *get_entry(const FormID &id, const QMap<QString, QVariant> &tags);
    bool is_complete(const QMap<QString, QVariant> &tags) const;
    bool all_values_in_range(const QMap<QString, QVariant> &tags) const;
    void from_json(const QJsonObject &object);
    bool section_exists(QString section_name);

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
    Data_engine(std::istream &source);

    void set_source(std::istream &source);
    bool is_complete(const QMap<QString, QVariant> &tags) const;
    bool all_values_in_range(const QMap<QString, QVariant> &tags) const;
    bool value_in_range(const FormID &id, const QMap<QString, QVariant> &tags) const;
    void set_actual_number(const FormID &id, const QMap<QString, QVariant> &tags, double number);
    void set_actual_text(const FormID &id, const QMap<QString, QVariant> &tags, QString text);
    void set_actual_bool(const FormID &id, const QMap<QString, QVariant> &tags, bool value);

    double get_desired_value(const FormID &id, const QMap<QString, QVariant> &tags) const;

    const QString &get_desired_text(const FormID &id, const QMap<QString, QVariant> &tags) const;
    const QString &get_unit(const FormID &id, const QMap<QString, QVariant> &tags) const;
    Statistics get_statistics() const;

    std::unique_ptr<QWidget> get_preview() const;
    void generate_pdf(const std::string &form, const std::__cxx11::string &destination) const;
    std::string get_json() const;

    QString get_desired_value_as_text(const FormID &id, const QMap<QString, QVariant> &tags) const;

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
