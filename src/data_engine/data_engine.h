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
enum class EntryType { Bool, String, Numeric };

enum class DataEngineErrorNumber {
    invalid_version_dependency_string,
    no_data_section_found,
    tolerance_parsing_error,
    duplicate_field,
    duplicate_section,
    non_unique_desired_field_found,
    no_section_id_found,
    faulty_field_id
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
    FormID field_name;

    virtual bool is_complete() const = 0;
    virtual bool is_in_range() const = 0;
    virtual QString get_value() const = 0;
    virtual QString get_description() const = 0;
    virtual QString get_minimum() const = 0;
    virtual QString get_maximum() const = 0;

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

    bool test_in_range(const double desired, const double measured);

    public:
    void from_string(const QString &str);
    QString to_string(const double desired_value);

    private:
    void str_to_num(QString str_in, double &number, ToleranceType &tol_type, bool &open, QStringList expected_sign_strings);
    QString num_to_str(double number, ToleranceType tol_type);

    ToleranceType tolerance_type;
    double deviation_limit_above;
    bool open_range_above;

    double deviation_limit_beneath;
    bool open_range_beneath;
};

struct NumericDataEntry : DataEngineDataEntry {
    NumericDataEntry(FormID field_name, double target_value, NumericTolerance tolerance, QString unit, QString description);
    bool valid() const;
    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_value() const override;
    QString get_description() const override;
    QString get_minimum() const override;
    QString get_maximum() const override;

    double get_min_value() const;
    double get_max_value() const;

    double target_value{};
    QString unit{};
    QString description{};
    std::experimental::optional<double> actual_value{};

    private:
    NumericTolerance tolerance;
};

struct TextDataEntry : DataEngineDataEntry {
    TextDataEntry(FormID name, QString target_value);

    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_value() const override;
    QString get_description() const override;
    QString get_minimum() const override;
    QString get_maximum() const override;

    QString target_value{};
    QString description{};
    std::experimental::optional<QString> actual_value{};
};

struct DependencyVersion {
    DependencyVersion();
    enum Match_style { MatchExactly, MatchByRange, MatchEverything, MatchNone };
    float version_number_match_exactly;
    float version_number_low_including;
    float version_number_high_excluding;
    Match_style match_style;

    public:
    void from_json(const QJsonValue &object, const bool default_to_match_all);
    bool is_matching(float version_number);
    void from_string(const QString &str);

    private:
    void from_number(const double &version_number);
    void parse_version_number(const QString &str, float &vnumber, bool &matcheverything);
};

struct DependencyVersions {
    QMultiMap<QString,DependencyVersion> versions;

    public:
    bool is_matching(float version_number);

    void from_json(const QJsonValue &object, const bool default_to_match_all, const QString &version_key_name);
};

struct DependencyTags {
    QMultiMap<QString, QVariant> tags;

    public:

    void from_json(const QJsonValue &object);
};

struct VariantData {
    DependencyTags dependency_tags;
    DependencyVersions versions_including;
    DependencyVersions versions_excluding;
    std::vector<std::unique_ptr<DataEngineDataEntry>> data_entries;

    public:
    bool is_matching_tags_n_versions(const QMultiMap<QString, QVariant> &tags, const QMap<QString, double> &versions_including_,
                                                      const QMap<QString, double> &versions_excluding_) const;
    void from_json(const QJsonObject &object);
    bool value_exists(QString field_name);
};

struct DataEngineSection {
    std::vector<VariantData> variants;

    public:
    const VariantData *get_variant(const QMultiMap<QString, QVariant> &tags, const QMap<QString, double> &versions_including,
                                   const QMap<QString, double> &versions_excluding) const;

    void from_json(const QJsonValue &object, const QString &key_name);
    QString get_section_name() const;

    private:
    QString section_name;
    void append_variant_from_json(const QJsonObject &object);
};

struct DataEngineSections {
    std::vector<DataEngineSection> sections;

    public:
    const DataEngineDataEntry *get_entry(const FormID &id, const QMultiMap<QString, QVariant> &tags, const QMap<QString, double> &versions_including,
                                         const QMap<QString, double> &versions_excluding) const;
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
    bool is_complete() const;
    bool all_values_in_range() const;
    bool value_in_range(const FormID &id) const;
    void set_actual_number(const FormID &id, double number);
    void set_actual_text(const FormID &id, QString text);

    double get_desired_value(const FormID &id, const QMultiMap<QString, QVariant> &tags, const QMap<QString, double> &versions_including,
                             const QMap<QString, double> &versions_excluding) const;
    double get_desired_absolute_tolerance(const FormID &id) const;
    double get_desired_relative_tolerance(const FormID &id) const;
    double get_desired_minimum(const FormID &id) const;
    double get_desired_maximum(const FormID &id) const;

    const QString &get_desired_text(const FormID &id) const;
    const QString &get_unit(const FormID &id) const;
    Statistics get_statistics() const;

    std::unique_ptr<QWidget> get_preview() const;
    void generate_pdf(const std::string &form, const std::__cxx11::string &destination) const;
    std::string get_json() const;

    private:
    void add_entry(std::pair<FormID, std::unique_ptr<DataEngineDataEntry>> &&entry);
    DataEngineDataEntry *get_entry(const FormID &id, const QMultiMap<QString, QVariant> &tags, const QMap<QString, double> &versions_including,
                                   const QMap<QString, double> &versions_excluding);
    const DataEngineDataEntry *get_entry(const FormID &id, const QMultiMap<QString, QVariant> &tags, const QMap<QString, double> &versions_including,
                                         const QMap<QString, double> &versions_excluding) const;
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
