#ifndef DATA_ENGINE_H
#define DATA_ENGINE_H

#include "exceptionalapproval.h"
#include "forward_decls.h"

#include <QDateTime>
#include <QJsonValue>
#include <QList>
#include <QString>
#include <QVariant>
#include <cassert>
#include <experimental/optional>
#include <istream>
#include <memory>
#include <string>
#include <vector>

/**
\ingroup configuration
\{
    \defgroup desired_value_database Desired Value Database
    \{
        \copydoc desired_value_database_content
    \}
\}
*/

/// \cond HIDDEN_SYMBOLS
class QJsonObject;
class QWidget;
class QVariant;
struct DataEngineSections;
class QSqlDatabase;
class QXmlStreamWriter;
struct DataEngineSection;
class ExceptionalApprovalDB;
class Communication_logger;
struct Sol_table;
struct MatchedDevice;
class QPlainTextEdit;
struct Console;
namespace sol {
    class state;
}

using FormID = QString;

class EntryType {
    public:
    enum { Unspecified, Bool, Text, Reference, Number, DateTime } t = EntryType::Unspecified;

    EntryType(decltype(t) me)
        : t{me} {}

    EntryType(QString str) {
        if (str == "Unspecified") {
            t = EntryType::Unspecified;
        } else if (str == "Bool") {
            t = EntryType::Bool;
        } else if (str == "Text") {
            t = EntryType::Text;
        } else if (str == "Reference") {
            t = EntryType::Reference;
        } else if (str == "Number") {
            t = EntryType::Number;
        } else if (str == "Datetime") {
            t = EntryType::DateTime;
        } else {
            assert(0);
        }
    }

    QString to_string() const {
        switch (t) {
            case EntryType::Unspecified:
                return "Unspecified";
            case EntryType::Bool:
                return "Bool";
            case EntryType::Text:
                return "Text";
            case EntryType::Reference:
                return "Reference";
            case EntryType::Number:
                return "Number";
            case EntryType::DateTime:
                return "Datetime";
        }
        return "Unspecified";
    }
};

enum class DataEngineErrorNumber {
    ok,
    invalid_version_dependency_string,
    no_data_section_found,
    invalid_data_entry_key,
    invalid_data_entry_type,
    invalid_json_object,
    invalid_json_file,
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
    reference_must_not_point_to_undefined_instance,
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
    dummy_mode_necessary,
    sql_error,
    cannot_open_file,
    actual_value_not_set,
    actual_value_is_not_a_number,
    desired_value_is_not_a_number,
    pdf_template_file_not_existing,
    inconsistant_types_across_variants,
    inconsistant_types_across_variants_and_reference_targets,
    datetime_dont_support_desired_values_yet,
    datetime_dont_support_references_yet,
    datetime_is_not_valid

};

QString select_newest_file_name(QStringList file_list, QString prefix);

class DataEngineActualValueStatisticFile {
    public:
    DataEngineActualValueStatisticFile();
    ~DataEngineActualValueStatisticFile();

    void start_recording(QString file_root_path, QString file_prefix);
    void set_actual_value(const FormID &field_name, const QString serialised_dependency, double value);
    void set_dut_identifier(QString dut_identifier);
    QString select_file_name_to_be_used(QStringList file_list);
    void save_to_file();

    private:
    void open_or_create_new_file();
    void open_file(QString file_name);

    QString file_root_path;
    QString file_prefix;
    QString used_file_name;
    QString dut_identifier;
    QJsonObject data_entries;
    const uint entry_limit = 100;
    bool lock_file_exists = false;
    bool is_opened = false;
    void close_file();
    void remove_lock_file();
    bool check_and_create_lock_file();
    void create_new_file();
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

enum class TextFieldDataBandPlace { report_header, report_footer, page_header, page_footer, none, all };
struct PrintOrderItem {
    QString section_name;
    bool print_enabled = true;
    bool print_as_text_field = false;
    uint text_field_column_count = 2;
    TextFieldDataBandPlace text_field_place{TextFieldDataBandPlace::report_header};
};

struct PrintOrderSectionItem {
    PrintOrderItem print_order_item;
    DataEngineSection *section{};
    QString field_name{};
    QString suffix{};
};

struct DataEngineDataEntry {
    DataEngineDataEntry(const FormID &field_name)
        : field_name(field_name)
        , exceptional_approval{} {}
    FormID field_name;

    virtual bool is_complete() const = 0;
    virtual bool is_in_range() const = 0;
    virtual QString get_actual_values() const = 0;
    virtual double get_actual_number() const = 0;
    virtual QString get_description() const = 0;
    virtual QString get_desired_value_as_string() const = 0;
    virtual QString get_unit() const = 0;
    virtual double get_si_prefix() const = 0;
    virtual void set_desired_value_from_desired(DataEngineDataEntry *from) = 0;
    virtual void set_desired_value_from_actual(DataEngineDataEntry *from) = 0;
    virtual bool is_desired_value_set() const = 0;
    virtual bool compare_unit_desired_siprefix(const DataEngineDataEntry *from) const = 0;
    virtual EntryType get_entry_type() const = 0;
    virtual QJsonObject get_specific_json_dump() const = 0;
    virtual QString get_specific_json_name() const = 0;
    virtual double get_desired_number() const = 0;
    void set_exceptional_approval(ExceptionalApprovalResult exceptional_approval);
    const ExceptionalApprovalResult &get_exceptional_approval() const;

    template <class T>
    T *as();
    template <class T>
    const T *as() const;
    static std::unique_ptr<DataEngineDataEntry> from_json(const QJsonObject &object);
    virtual ~DataEngineDataEntry() = default;

    private:
    ExceptionalApprovalResult exceptional_approval{};
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
    QJsonObject get_json(double desirec_value) const;
    bool is_inherited_by_reference_targed = false;

    private:
    bool is_undefined = true;
    void str_to_num(QString str_in, double &number, ToleranceType &tol_type, bool &open, QStringList expected_sign_strings);
    QString num_to_str(double number, ToleranceType tol_type) const;

    ToleranceType tolerance_type;
    double deviation_limit_above;
    bool open_range_above;

    double deviation_limit_beneath;
    bool open_range_beneath;

    double get_absolute_limit_beneath(const double desired) const;
    double get_absolute_limit_above(const double desired) const;
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
    double get_actual_number() const override;
    double get_desired_number() const override;
    bool compare_unit_desired_siprefix(const DataEngineDataEntry *from) const override;
    void set_actual_value(double actual_value);
    EntryType get_entry_type() const override;
    bool is_desired_value_set() const override;
    NumericTolerance get_tolerance() const;
    QJsonObject get_specific_json_dump() const override;
    QString get_specific_json_name() const override;

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
    double get_actual_number() const override;
    QString get_description() const override;
    QString get_desired_value_as_string() const override;
    QString get_unit() const override;
    double get_si_prefix() const override;
    double get_desired_number() const override;
    bool compare_unit_desired_siprefix(const DataEngineDataEntry *from) const override;
    void set_actual_value(QString actual_value);
    EntryType get_entry_type() const override;
    bool is_desired_value_set() const override;

    QJsonObject get_specific_json_dump() const override;
    QString get_specific_json_name() const override;

    std::experimental::optional<QString> desired_value{};
    QString description{};

    private:
    void set_desired_value_from_desired(DataEngineDataEntry *from) override;
    void set_desired_value_from_actual(DataEngineDataEntry *from) override;

    std::experimental::optional<QString> actual_value{};
};

struct DataEngineDateTimeFormatPrecisionPair;

struct DateTimeFormatPrecision {
    QString format;
    enum precision_m_type { none, date_time_ms, date_time_s, date_time, date } precision_m;
};

struct DataEngineDateTime {
    public:
    DataEngineDateTime()
        : precision_m{DateTimeFormatPrecision::none}
        , dt_m(QDateTime()) {}

    DataEngineDateTime(QString text);
    DataEngineDateTime(QDate date);
    DataEngineDateTime(QDateTime datetime);
    DataEngineDateTime(double secs_since_epoch);
    bool isValid() const;
    QDateTime dt() const;
    QString str() const;
    static QStringList allowed_formats();

    protected:
    DateTimeFormatPrecision::precision_m_type precision_m = DateTimeFormatPrecision::none;
    QDateTime dt_m{};

    static QList<DateTimeFormatPrecision> formats();
};
Q_DECLARE_METATYPE(DataEngineDateTime);

struct DateTimeDataEntry : DataEngineDataEntry {
    DateTimeDataEntry(const DateTimeDataEntry &other);

    DateTimeDataEntry(const FormID name, QString description);

    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_actual_values() const override;
    double get_actual_number() const override;
    QString get_description() const override;
    QString get_desired_value_as_string() const override;
    QString get_unit() const override;
    double get_si_prefix() const override;
    double get_desired_number() const override;
    bool compare_unit_desired_siprefix(const DataEngineDataEntry *from) const override;
    void set_actual_value(DataEngineDateTime actual_value);
    EntryType get_entry_type() const override;
    bool is_desired_value_set() const override;

    QJsonObject get_specific_json_dump() const override;
    QString get_specific_json_name() const override;

    // std::experimental::optional<QDateTime> desired_value{};
    QString description{};

    private:
    void set_desired_value_from_desired(DataEngineDataEntry *from) override;
    void set_desired_value_from_actual(DataEngineDataEntry *from) override;

    // NumericTolerance tolerance;
    std::experimental::optional<DataEngineDateTime> actual_value{};
};

struct BoolDataEntry : DataEngineDataEntry {
    BoolDataEntry(const BoolDataEntry &other);
    BoolDataEntry(const FormID name, std::experimental::optional<bool> desired_value, QString description);

    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_actual_values() const override;
    double get_actual_number() const override;
    QString get_description() const override;
    QString get_desired_value_as_string() const override;
    QString get_unit() const override;
    double get_si_prefix() const override;
    double get_desired_number() const override;
    bool compare_unit_desired_siprefix(const DataEngineDataEntry *from) const override;
    void set_actual_value(bool value);
    EntryType get_entry_type() const override;
    bool is_desired_value_set() const override;

    QJsonObject get_specific_json_dump() const override;
    QString get_specific_json_name() const override;

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

    // referenz erbt typ vom target
    // referenz erbt unit vom target
    // referenz si-prefix unit vom target
    // referenz erbt sollwert von target, dies kann jeweils enweder soll oder ist
    // wert sein.
    bool is_complete() const override;
    bool is_in_range() const override;
    QString get_actual_values() const override;
    double get_actual_number() const override;
    QString get_description() const override;
    QString get_desired_value_as_string() const override;
    QString get_unit() const override;
    double get_si_prefix() const override;
    double get_desired_number() const override;
    EntryType get_entry_type() const override;
    bool compare_unit_desired_siprefix(const DataEngineDataEntry *from) const override;
    void set_actual_value(double number);
    void set_actual_value(QString val);
    void set_actual_value(DataEngineDateTime val);
    void set_actual_value(bool val);
    void dereference(DataEngineSections *sections, const bool is_dummy_mode);
    bool is_desired_value_set() const override;

    QJsonObject get_specific_json_dump() const override;
    QString get_specific_json_name() const override;

    NumericTolerance tolerance;
    QString description{};

    std::vector<ReferenceLink> reference_links;

    private:
    void set_desired_value_from_desired(DataEngineDataEntry *from) override;
    void set_desired_value_from_actual(DataEngineDataEntry *from) override;
    void update_desired_value_from_reference() const;
    void parse_refence_string(QString reference_string);
    bool not_defined_yet_due_to_undefined_instance_count = false;
    void assert_that_instance_count_is_defined() const;

    DataEngineDataEntry *entry_target = nullptr;
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
    QString get_serialised_string() const;
    void from_json(const QJsonValue &object, const bool default_to_match_all);
    bool is_matching(const QVariant &test_value) const;
    void from_string(const QString &str);

    private:
    void from_number(const double &number);
    void from_bool(const bool &boolean);
    void parse_number(const QString &str, float &vnumber, bool &matcheverything);
    QString serialised_string;
};

struct DependencyTags {
    QMultiMap<QString, DependencyValue> tags;

    public:
    QString get_dependencies_serialised_string() const;
    void from_json(const QJsonValue &object);
};

struct VariantData {
    VariantData();
    VariantData(const VariantData &other);

    // VariantData(const VariantData &) = delete;
    VariantData &operator=(const VariantData &) = delete;
    ~VariantData() = default;

    DependencyTags dependency_tags;
    std::vector<std::unique_ptr<DataEngineDataEntry>> data_entries;
    uint get_entry_count() const;

    public:
    QString get_dependencies_serialised_string() const;
    bool uses_dependency() const;
    bool is_dependency_matching(const QMap<QString, QList<QVariant>> &tags, uint instance_index, uint instance_count, const QString &section_name);
    void from_json(const QJsonObject &object);
    bool entry_exists(QString field_name);
    DataEngineDataEntry *get_entry(QString field_name) const;
    DataEngineDataEntry *get_entry_raw(QString field_name, DataEngineErrorNumber *errornum) const;

    const QMap<QString, QVariant> &get_relevant_dependencies() const;

    private:
    QMap<QString, QVariant> relevant_dependencies;
};

struct DataEngineInstance {
    DataEngineInstance();
    DataEngineInstance(const DataEngineInstance &other);
    DataEngineInstance(DataEngineInstance &&other); // move constructor

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
    QString get_sql_section_name() const;
    QString get_sql_instance_name() const;
    QString get_actual_instance_caption() const;
    QStringList get_all_ids_of_selected_instance(const QString &prefix) const;
    QStringList get_instance_captions() const;
    bool is_section_instance_defined() const;

    bool set_instance_count_if_name_matches(QString instance_count_name, uint instance_count);
    void use_instance(QString instance_caption, uint instance_index);
    void set_actual_instance_caption(QString instance_caption);
    void create_instances_if_defined();

    QStringList get_all_ids_over_variants() const;
    void set_actual_number(const FormID &id, double number);
    void set_actual_text(const FormID &id, QString text);
    void set_actual_bool(const FormID &id, bool value);
    void set_actual_datetime(const FormID &id, DataEngineDateTime value);

    DataEngineInstance prototype_instance;

    uint get_actual_instance_index();

    QString get_section_title() const;

    bool section_uses_variants() const;

    QString get_serialised_dependency_string() const;

    private:
    std::experimental::optional<uint> instance_count;

    QString section_title;
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
    QList<const DataEngineDataEntry *> get_entries_const(const FormID &id, const bool using_dummy_mode) const;
    QList<const DataEngineDataEntry *> get_entries_across_variants_const(const FormID &id) const;
    const DataEngineDataEntry *get_actual_instance_entry_const(const FormID &id) const;
    QList<DataEngineDataEntry *> get_entries(const FormID &id);
    bool exists_uniquely(const FormID &id, const bool using_dummy_mode) const;
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
    const QMap<QString, QList<QVariant>> &get_dependancy_tags() const;
    bool is_dummy_data_mode = true;

    DataEngineSection *get_section_no_exception(FormID id) const;

    EntryType get_entry_type_dummy_mode(const FormID &id) const;

    private:
    QList<const DataEngineDataEntry *> get_entries_raw(const FormID &id, DataEngineErrorNumber *error_num, DecodecFieldID &decoded_field_name,
                                                       bool using_instance_index, bool dummy_mode) const;
    DataEngineSection *get_section_raw(const QString &section_name, DataEngineErrorNumber *error_num) const;
    QMap<QString, QList<QVariant>> dependency_tags;
    EntryType get_entry_type_dummy_mode_recursion(const FormID &id) const;
};
/// \endcond

/** \defgroup data_engine Data- and report engine
 *  Interface of built-in user interface functions.
 *  \{
 */

// clang-format off
/*!
  \class   Data_engine
  \brief  Interface to the DataEngine
  */
// clang-format on

class Data_engine {
    struct Statistics {
        int number_of_id_fields{};
        int number_of_data_fields{};
        int number_of_filled_fields{};
        int number_of_inrange_fields{};
        QString to_qstring() const;
    };

    public:
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    Data_engine(string pdf_report_template_path, string desired_values_path, string auto_dump_path, table dependency_tags);
#endif
    /// \cond HIDDEN_SYMBOLS
    Data_engine();
    Data_engine(std::istream &source, const QMap<QString, QList<QVariant>> &tags);
    Data_engine(std::istream &source); // for getting dummy data structure
    ~Data_engine();
    /// \endcond
    // clang-format off
/*! \fn Data_engine(string pdf_report_template_path, string desired_values_path, string auto_dump_path, table dependency_tags);
    \brief Creates a Data_engine object based on the \ref desired_value_database desired_values database.
    \param pdf_report_template_path the path the the report template which will be used to generate a pdf report.
    \param desired_values_path the path the desired_value \glos{json} database.
    \param auto_dump_path the directory in which the machine-readable \glos{json} version of the pdf is stored.
    \param dependency_tags the tags used the choose the right variant of the desired value section.

    \sa desired_value_database

     \par examples:
     \code
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    data_engine:set_open_pdf_on_pdf_creation(true)
    data_engine:set_start_time_seconds_since_epoch(current_date_time_ms()/1000)

    data_engine:set_actual_datetime("allgemein/datum_today",    os.time(t))
    data_engine:set_actual_text("test_version/git_framework",	get_framework_git_hash())
    data_engine:set_actual_number("gerate_daten/seriennummer",	12568)
    data_engine:set_actual_bool("gerate_daten/bool_test1",		true)

    if data_engine:all_values_in_range() then
        show_info("Ready", "Test Successfull")
    end

    \endcode
*/


    /// \cond HIDDEN_SYMBOLS
    void enable_logging(Console &console, const std::vector<MatchedDevice> &devices);
    void set_log_file(const std::string &file_path);
    void set_dependancy_tags(const QMap<QString, QList<QVariant>> &tags);
    void set_source(std::istream &source);
    void set_script_path(QString script_path);

    QString source_path;

    void save_actual_value_statistic();
    QStringList get_instance_count_names();
    /// \endcond






#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    void set_dut_identifier(string dut_identifier);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_dut_identifier(QString dut_identifier);
    /// \endcond
    // clang-format off
/*! \fn void set_dut_identifier(string dut_identifier);
    \brief When creating actual value statistics files it is necessary to identify each device under test. This function sets
        this identifier. Could be a GUID or a serialnumber

    \param dut_identifier string to identify the current device under test(serialnumber, GUID etc)
    \sa start_recording_actual_value_statistic()
     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local dut_serial_number = "1234567"
    data_engine:start_recording_actual_value_statistic(PROJECT_2000_PATH.."_statistic\\project_2000\\","actual_value_stat")
    data_engine:set_dut_identifier(dut_serial_number)
\endcode
*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    void start_recording_actual_value_statistic(string root_file_path, string file_name_prefix);
#endif
    /// \cond HIDDEN_SYMBOLS
    void start_recording_actual_value_statistic(const std::string &root_file_path, const std::string &file_prefix);
    /// \endcond
    // clang-format off
/*! \fn void start_recording_actual_value_statistic(string root_file_path, string file_name_prefix);
    \brief It is possible to save each actual value written into the data_engine in a file. This way you can analyze later
    the value destribution even if values outside the tolerances were measured. This is particulary helpful if you measure
    multiple times. Calling this function enables the feature.
    \param root_file_path The Rootpath for the file.
    \param file_name_prefix The file name prefix of the logged data.
    \sa set_dut_identifier()
     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local dut_serial_number = "1234567"
    data_engine:start_recording_actual_value_statistic(PROJECT_2000_PATH.."_statistic\\project_2000\\","actual_value_stat")
    data_engine:set_dut_identifier(dut_serial_number)
\endcode
*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_start_time_seconds_since_epoch(number start_seconds_since_epoch);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_start_time_seconds_since_epoch(double start_seconds_since_epoch);
    /// \endcond
    // clang-format off
/*! \fn set_start_time_seconds_since_epoch(number start_seconds_since_epoch);
    \brief Sets the starting time of the script or test execution. If this is set the total runtime will
        be calculated and written into the machine readable data dump file alongside the other measurement results.
    \param start_seconds_since_epoch The current time since epoch in seconds.

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    data_engine:set_start_time_seconds_since_epoch(current_date_time_ms()/1000)

    sleep_ms(5000)
\endcode

The result will be documented in the final data dump as:
     \code{.json}
{
    [..]
    "general": {
       [..],
        "test_duration_seconds": 5,
       [..]
    },
    [..]

}
    \endcode
*/
    /// \cond HIDDEN_SYMBOLS
    bool is_complete() const;
    bool value_complete(const FormID &id) const;
    bool value_complete_in_instance(const FormID &id) const;
    bool value_complete_in_section(FormID id) const;
  /// \endcond

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    bool value_in_range_in_instance(string data_engine_field);
#endif
    /// \cond HIDDEN_SYMBOLS
    bool value_in_range_in_instance(const FormID &id) const;
    /// \endcond
    // clang-format off
/*! \fn   bool value_in_range_in_instance(string data_engine_field);
    \brief returns true if all actual value is within the tolerance+desired value. In case of multi instance
         sections only the data field of the currently selected section is considered.
    \param data_engine_field  The datafield-ID
    \returns true if this actual value is in tolerance
     \sa desired_value_database for understanding the instance concept.

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)
    data_engine:set_actual_number("desired_values/test_number_a", 0.101);
    local result = data_engine:value_in_range_in_instance("desired_values/test_number_a")
    -- result is true
\endcode
\code{.json}
{
    "desired_values": {
        "title":"Measured values",
        "data":[
        {	"name": "test_number_a",
            "value": 100,
            "tolerance":1 ,
            "unit":"mA",
            "si_prefix":1e-3,
            "nice_name": "A number field"
        }
       ]
    }
}
\endcode
*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    bool value_in_range_in_section(string section_name);
#endif
    /// \cond HIDDEN_SYMBOLS
    bool value_in_range_in_section(FormID id) const;
    /// \endcond
    // clang-format off
/*! \fn   bool value_in_range_in_section(string section_name);
    \brief returns true if all actual values of the specified section are within the tolerance+desired value.
    \param section_name The section name of the section to be checked
    \returns true if actual values are in tolerance

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)
    data_engine:set_actual_number("desired_values/test_number_a", 0.101);
    data_engine:set_actual_number("desired_values/test_number_b", 0.105);
    local result = data_engine:value_in_range_in_section("desired_values")
    -- result is true
\endcode
\code{.json}
{
    "desired_values": {
        "title":"Measured values",
        "data":[
        {	"name": "test_number_a",
            "value": 100,
            "tolerance":1 ,
            "unit":"mA",
            "si_prefix":1e-3,
            "nice_name": "A number field"
        }, {
            "name": "test_number_b",
            "value": 105,
            "tolerance":1 ,
            "unit":"mA",
            "si_prefix":1e-3,
            "nice_name": "A number field"
        }
       ]
    }
}
\endcode
*/


#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    bool all_values_in_range();
#endif
    /// \cond HIDDEN_SYMBOLS
    bool all_values_in_range() const;
    /// \endcond
    // clang-format off
/*! \fn bool all_values_in_range();
    \brief Returns true if all actual values are set and within their tolerances.
    \returns true if all actual values are set and in tolerance

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local result = data_engine:all_values_in_range()
    -- result is true / false
\endcode

*/


#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    bool values_in_range(string_table data_engine_fields);
#endif
    /// \cond HIDDEN_SYMBOLS
    bool values_in_range(const QList<FormID> &ids) const;
    /// \endcond
    // clang-format off
/*! \fn bool values_in_range(string_table data_engine_fields);
    \brief returns true if the actual values of a list of data field is within the tolerance+desired value.
    \param data_engine_fields The list of datafield-IDs to check
    \returns true if specified actual values are in tolerance

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)
    data_engine:set_actual_number("desired_values/test_number_a", 0.101);
    data_engine:set_actual_number("desired_values/test_number_b", 0.105);
    local result = data_engine:values_in_range({"desired_values/test_number_a","desired_values/test_number_b"})
    -- result is true
\endcode
\code{.json}
{
    "desired_values": {
        "title":"Measured values",
        "data":[
        {	"name": "test_number_a",
            "value": 100,
            "tolerance":1 ,
            "unit":"mA",
            "si_prefix":1e-3,
            "nice_name": "A number field"
        }, {
            "name": "test_number_b",
            "value": 105,
            "tolerance":1 ,
            "unit":"mA",
            "si_prefix":1e-3,
            "nice_name": "A number field"
        }
       ]
    }
}
\endcode
*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    bool value_in_range(string data_engine_field);
#endif
    /// \cond HIDDEN_SYMBOLS
    bool value_in_range(const FormID &id) const;
    /// \endcond
    // clang-format off
/*! \fn bool value_in_range(string data_engine_field);
    \brief returns true if the actual value of a data field is within the tolerance+desired value. If multiple instances are
    used there are also multiple fields being addressed by the field ID. In this case it consideres all of them and returns
     true of all fields are in range.
    \param data_engine_field The datafield-ID
    \returns true if this actual value is in tolerance
     \sa desired_value_database for understanding the instance concept.
     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)
    data_engine:set_actual_number("desired_values/test_number", 0.101);
    local result = data_engine:value_in_range("desired_values/test_number")
    -- result is true
\endcode
\code{.json}
{
    "desired_values": {
        "title":"Measured values",
        "data":[
        {	"name": "test_number",
            "value": 100,
            "tolerance":1 ,
            "unit":"mA",
            "si_prefix":1e-3,
            "nice_name": "A number field"
        }
       ]
    }
}
\endcode
*/



#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    bool is_exceptionally_approved(string data_engine_field);
#endif
    /// \cond HIDDEN_SYMBOLS
    bool is_exceptionally_approved(const FormID &id) const;
    /// \endcond
    // clang-format off
/*! \fn bool is_exceptionally_approved(string data_engine_field);
    \brief returns true if the data field has been exceptionally approved by the user
    \param data_engine_field The datafield-ID
    \returns true if there is an exceptional approval for that field.

       \sa exceptional_approval
     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local result = data_engine:is_exceptionally_approved("desired_values/test_datetime")
    -- result is false
\endcode
*/



#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    bool is_datetime(string data_engine_field);
#endif
    /// \cond HIDDEN_SYMBOLS
    bool is_datetime(const FormID &id) const;
    /// \endcond
    // clang-format off
/*! \fn bool is_datetime(string data_engine_field);
    \brief returns true if the data field is a datetime type.
    \param data_engine_field The datafield-ID
    \returns true if data field is of type datetime

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local result = data_engine:is_datetime("desired_values/test_datetime")
    -- result is true
\endcode
*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    bool is_bool(string data_engine_field);
#endif
    /// \cond HIDDEN_SYMBOLS
    bool is_bool(const FormID &id) const;
    /// \endcond
    // clang-format off
/*! \fn bool is_bool(string data_engine_field);
    \brief returns true if the data field is a bool type.
    \param data_engine_field The datafield-ID
    \returns true if data field is of type bool

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local result = data_engine:is_bool("desired_values/test_bool")
    -- result is true
\endcode
*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    bool is_text(string data_engine_field);
#endif
    /// \cond HIDDEN_SYMBOLS
    bool is_text(const FormID &id) const;
    /// \endcond
    // clang-format off
/*! \fn bool is_text(string data_engine_field);
    \brief returns true if the data field is a string type.
    \param data_engine_field The datafield-ID
    \returns true if data field is of type string

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local result = data_engine:is_text("desired_values/test_text")
    -- result is true
\endcode
*/



#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    bool is_number(string data_engine_field);
#endif
    /// \cond HIDDEN_SYMBOLS
    bool is_number(const FormID &id) const;
    /// \endcond
    // clang-format off
/*! \fn bool is_number(string data_engine_field);
    \brief returns true if the data field is a number type.
    \param data_engine_field The datafield-ID
    \returns true if data field is of type number

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local number_result = data_engine:is_number("desired_values/test_number")
    -- number_result is true
\endcode
*/


#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_actual_number(string data_engine_field, number value);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_actual_number(const FormID &id, double number);
    /// \endcond
    // clang-format off
/*! \fn set_actual_number(string data_engine_field, number value);
    \brief Sets the actual value of a number field
    \param data_engine_field The datafield-ID
    \param value The value is always set in its base unit(e.g Ampere) derived by the si_prefix.
            If the desired value has the unit e.g milli Ampere and thus the si_prefix 1e-3, you have to
            check-in the value 0.101A if you have measured 101mA.

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    data_engine:set_actual_number("desired_values/test_number", 0.101);
    local number_result = data_engine:get_actual_number("desired_values/test_number")
    -- number_result is 101.0

\endcode
     \code{.json}
{
    "desired_values": {
        "title":"Measured values",
        "data":[
        {	"name": "test_number",
            "value": 100,
            "tolerance":1 ,
            "unit":"mA",
            "si_prefix":1e-3,
            "nice_name": "A number field"
        }
       ]
    }
}
    \endcode
*/


#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_actual_text(string data_engine_field, string value);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_actual_text(const FormID &id, QString text);
    /// \endcond
    // clang-format off
/*! \fn set_actual_text(string data_engine_field, string value);
    \brief Sets the actual value of a string field
    \param data_engine_field The datafield-ID
    \param value The value to be set.

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    data_engine:set_actual_text("desired_values/test_text", "test123");
    local text_result = data_engine:get_actual_value("desired_values/test_text")
    --result is "test123"
\endcode
     \code{.json}
{
    "desired_values": {
        "title":"Measured values",
        "data":[
        {	"name": "test_text",
            "value": "test123",
            "nice_name": "A Text field"
        }
       ]
    }
}
    \endcode
*/





#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_actual_bool(string data_engine_field, bool value);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_actual_bool(const FormID &id, bool value);
    /// \endcond
    // clang-format off
/*! \fn set_actual_bool(string data_engine_field, bool value);
    \brief Sets the actual value of a boolean field
    \param data_engine_field The datafield-ID
    \param value The value to be set.

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    data_engine:set_actual_bool("desired_values/test_bool", true);

\endcode
     \code{.json}
{
    "desired_values": {
        "title":"Measured values",
        "data":[
        {	"name": "test_bool",
            "value": true,
            "nice_name": "A bool field"
        }
       ]
    }
}
    \endcode
*/



#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_open_pdf_on_pdf_creation(bool open_pdf_automatically);
#endif
/*! \fn set_open_pdf_on_pdf_creation(bool open_pdf_automatically);
    \brief With this function you can tell the data_engine to open the pdf after report creation.
    \param open_pdf_automatically if true, the pdf is opened after finishing


     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    data_engine:set_open_pdf_on_pdf_creation(true);
    --opens pdf when script finishes
\endcode

*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    add_extra_pdf_path(string file_name);
#endif
/*! \fn add_extra_pdf_path(string file_name);
    \brief With this function you can tell the data_engine to generate a copy of the pdf report.
    \param file_name Where the copy of the pdf report is stored to.

     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    data_engine:add_extra_pdf_path("path\\to\\another_pdf_report.pdf");

\endcode

*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_actual_datetime_from_text(string data_engine_field, string value);
#endif
/*! \fn set_actual_datetime_from_text(string data_engine_field, number seconds_since_epoch);
    \brief Sets the actual value of a date time field using a string
    \param data_engine_field The datafield-ID
    \param value The value to be set.
    Following formats are allowed:
        - "yyyy.MM.dd"
        - "yyyy:MM:dd HH:mm:ss"
        - "yyyy-MM-dd HH:mm:ss"
        - "yyyy-MM-dd"
        - "yyyy-MM-dd hh:mm"
        - "yyyy-MM-dd hh:mm:ss"
        - "yyyy-MM-dd hh:mm:ss.zzz"


     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    data_engine:set_actual_datetime_from_text("desired_values/test_datetime", "2018-12-19 12:50:01 +0100");
    local date_result = data_engine:get_actual_value("desired_values/date_today")
    -- date_result is eg "2018-12-19 12:50:01"
\endcode
     \code{.json}
{
    "desired_values": {
        "title":"Measured values",
        "data":[
        {	"name": "date_today",	 "type": "datetime",    "nice_name": "A Datetime field"	}
       ]
    }
}
    \endcode
*/



#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_actual_datetime(string data_engine_field, number seconds_since_epoch);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_actual_datetime(const FormID &id, DataEngineDateTime value);
    /// \endcond
    // clang-format off
/*! \fn set_actual_datetime(string data_engine_field, number seconds_since_epoch);
    \brief Sets the actual value of a date time field
    \param data_engine_field The datafield-ID
    \param seconds_since_epoch Value in unix time(seconds since epoch).

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    data_engine:set_actual_datetime("desired_values/test_datetime", os.time());
    local date_result = data_engine:get_actual_value("desired_values/date_today")
    -- date_result is eg "2018-01-01 13:45:50"
\endcode
     \code{.json}
{
    "desired_values": {
        "title":"Measured values",
        "data":[
        {	"name": "date_today",	 "type": "datetime",    "nice_name": "A Datetime field"	}
       ]
    }
}
    \endcode
*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    use_instance(string section_name, string instance_caption, number instance_index);
#endif
    /// \cond HIDDEN_SYMBOLS
    void use_instance(const QString &section_name, const QString &instance_caption, const uint instance_index);
    /// \endcond
    // clang-format off
/*! \fn  use_instance(string section_name, string instance_caption, number instance_index);
    \brief selects the current section's instance index.
    \param section_name The name of the section which uses the multi instance concept
    \param instance_caption In the pdf report each instance can have a dedicated title which is defined here.
    \param instance_index the index of the currently selected instance.

    \sa desired_value_database for understanding the instance concept.
    \sa set_instance_count()
    \sa get_instance_count()

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local battery_count = 10
    data_engine:set_instance_count("battery_test_count",battery_count);
    local battery_index = 1
    for i,v in pairs(batteries) do
        data_engine:use_instance("battery_test", "Battery SN: "..v.serial_number, battery_index)

        data_engine:set_actual_text("battery_test/seriennummer",	v.serialnumber)
        data_engine:set_actual_number("battery_test/voltage",		v.voltage)

        battery_index = battery_index+1
    end
    \endcode

    \code{.json}
    {
        "battery_test":{
            "title":"Delivered batteries",
            "instance_count":"battery_test_count",
            "data":[
                {
                    "name": "seriennummer",
                    "type": "text",	 "nice_name": "Seriennummer"
                },{
                    "name": "voltage",
                    "value": 100,
                    "unit": "mV",
                    "tolerance": "+3/-9",
                    "si_prefix": 1000,
                    "nice_name": "Fester Strom Toleranz 1"
                }
            ]
        },
    }
    \endcode
*/


#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_instance_count(string instance_count_alias, number instance_count);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_instance_count(QString instance_count_name, uint instance_count);
    /// \endcond
    // clang-format off
/*! \fn  set_instance_count(string instance_count_alias, number instance_count);
    \brief sets the instance count value of the corresponding alias.
    \param instance_count_alias The instance count alias
    \param instance_count How many instances are used.


    \sa desired_value_database for understanding the instance concept.
    \sa use_instance()
    \sa get_instance_count()
     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local battery_count = 10
    data_engine:set_instance_count("battery_test_count",battery_count);
    local battery_index = 1
    for i,v in pairs(batteries) do
        data_engine:use_instance("battery_test", "Battery SN: "..v.serial_number, battery_index)

        data_engine:set_actual_text("battery_test/seriennummer",	v.serialnumber)
        data_engine:set_actual_number("battery_test/voltage",		v.voltage)

        battery_index = battery_index+1
    end
    \endcode

    \code{.json}
    {
        "battery_test":{
            "title":"Delivered batteries",
            "instance_count":"battery_test_count",
            "data":[
                {
                    "name": "seriennummer",
                    "type": "text",	 "nice_name": "Seriennummer"
                },{
                    "name": "voltage",
                    "value": 100,
                    "unit": "mV",
                    "tolerance": "+3/-9",
                    "si_prefix": 1000,
                    "nice_name": "Fester Strom Toleranz 1"
                }
            ]
        },
    }
    \endcode
*/




#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    number get_instance_count(string section_name);
#endif
    /// \cond HIDDEN_SYMBOLS
    uint get_instance_count(const std::string &section_name) const;
    /// \endcond
    // clang-format off
/*! \fn  number get_instance_count(string section_name);
    \brief returns how many instances one section uses.
    \param section_name The section's name
    \returns how many instances the specified section uses.


    \sa desired_value_database for understanding the instance concept.
    \sa use_instance()
    \sa set_instance_count()

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local result = data_engine:get_instance_count("battery_test");
    --result == 1
    \endcode

    \code{.json}
    {
        "battery_test":{
            "title":"Delivered batteries",
            "data":[
                {
                    "name": "seriennummer",
                    "type": "text",	 "nice_name": "Seriennummer"
                },{
                    "name": "voltage",
                    "value": 100,
                    "unit": "mV",
                    "tolerance": "+3/-9",
                    "si_prefix": 1000,
                    "nice_name": "Fester Strom Toleranz 1"
                }
            ]
        },
    }
    \endcode
*/



#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    string get_actual_value(string data_engine_field);
#endif
    /// \cond HIDDEN_SYMBOLS
    QString get_actual_value(const FormID &id) const;
    /// \endcond
    // clang-format off
/*! \fn string get_actual_value(string data_engine_field);
    \brief Returns the the actual value text of one data field as it appears in the pdf report.
    \param data_engine_field The datafield-ID
    \returns text version of the actual value stored in the data field

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)


    data_engine:set_actual_datetime("desired_values/test_datetime", os.time());
    data_engine:set_actual_number("desired_values/test_number", 0.101);
    data_engine:set_actual_bool("desired_values/test_bool", true);
    data_engine:set_actual_text("desired_values/test_string", "test123");


    local date_result = data_engine:get_actual_value("desired_values/test_datetime")
    local number_result = data_engine:get_actual_value("desired_values/test_number")
    local bool_result = data_engine:get_actual_value("desired_values/test_bool")
    local text_result = data_engine:get_actual_value("desired_values/test_string")

    -- number_result is "101 mA"
    -- bool_result is "Yes"
    -- text_result is "test123"
    -- date_result is eg "2018-01-01 13:45:50"
\endcode
     \code{.json}
{
    "desired_values": {
        "title":"Measured values",
        "data":[
        {	"name": "date_today",	 "type": "datetime",    "nice_name": "A Datetime field"	},
        {	"name": "test_bool",	 "type": "bool",        "nice_name": "A boolean field"	},
        {	"name": "test_string",	 "type": "string",      "nice_name": "A text field"     },
        {	"name": "test_number",
            "value": 100,
            "tolerance":1 ,
            "unit":"mA",
            "si_prefix":1e-3,
            "nice_name": "A number field"
        }
       ]
    }
}
    \endcode
*/



#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    number get_actual_number(string data_engine_field);
#endif
    /// \cond HIDDEN_SYMBOLS
        double get_actual_number(const FormID &id) const;
    /// \endcond
    // clang-format off
/*! \fn number get_actual_number(string data_engine_field);
    \brief Returns the the actual number of one data field. If the specified field is not of type "number" an error is thrown
    \param data_engine_field The datafield-ID
    \returns actual value stored in the data field

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    data_engine:set_actual_number("desired_values/test_number", 0.101);
    local number_result = data_engine:get_actual_number("desired_values/test_number")
    -- number_result is 101.0

\endcode
     \code{.json}
{
    "desired_values": {
        "title":"Measured values",
        "data":[
        {	"name": "test_number",
            "value": 100,
            "tolerance":1 ,
            "unit":"mA",
            "si_prefix":1e-3,
            "nice_name": "A number field"
        }
       ]
    }
}
    \endcode
*/



#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    number get_desired_number(string data_engine_field);
#endif
    /// \cond HIDDEN_SYMBOLS
double get_desired_number(const FormID &id) const;
    /// \endcond
    // clang-format off
/*! \fn number get_desired_number(string data_engine_field);
    \brief Returns the desired value of a datafield as a number if the field is of type number.
    \param data_engine_field The datafield-ID


     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local result_a = data_engine:get_desired_number("measurements/current")
    --result_a: 100.0

    \endcode
With the \link desired_value_database desired value file \endlink:
     \code{.json}
{
    "measurements":{
        "title":"Measurements",
        "data":[
            {	"name": "current",
                "value": 100,
                "unit": "mA",
                "tolerance": "+3/-9",
                "si_prefix": 1e-3,
                "nice_name":    "Suppy Current"}
        ]
    }
}
    \endcode
*/





#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    string get_description(string data_engine_field);
#endif
    /// \cond HIDDEN_SYMBOLS
    QString get_description(const FormID &id) const;
    /// \endcond
    // clang-format off
/*! \fn string get_description(string data_engine_field);
    \brief Returns the nice-name property of the data field
    \param data_engine_field The datafield-ID
    \returns nice-name property of datafield

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local description = data_engine:get_description("test_version/git_protokoll")
    --description contains: "Git-Hash Protokoll"

    \endcode
With the \link desired_value_database desired value file \endlink:
     \code{.json}
{
    "test_version":{
        "title":"Protokollversion",
        "data":[
            {	"name": "git_protokoll", "type": "string",	"nice_name":   "Git-Hash Protokoll"}
        ]
    }
}
    \endcode
*/


#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    string get_desired_value(string data_engine_field);
#endif
    /// \cond HIDDEN_SYMBOLS
    QString get_desired_value_as_string(const FormID &id) const;
    /// \endcond
    // clang-format off
/*! \fn string get_desired_value(string data_engine_field);
    \brief Returns the desired value of a datafield as text.
    \param data_engine_field The datafield-ID


     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local result_a = data_engine:get_desired_value("measurements/current")
    local result_b = data_engine:get_desired_value("measurements/text_test")
    --result_a: "100 mA"
    --result_b: "example"

    \endcode
With the \link desired_value_database desired value file \endlink:
     \code{.json}
{
    "measurements":{
        "title":"Measurements",
        "data":[
            {	"name": "current",
                "value": 100,
                "unit": "mA",
                "tolerance": "+3/-9",
                "si_prefix": 1e-3,
                "nice_name":    "Suppy Current"},
            {	"name": "text_test", "value": "example",	"nice_name":   "example description"}
        ]
    }
}
    \endcode
*/




#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    string get_unit(string data_engine_field);
#endif
    /// \cond HIDDEN_SYMBOLS
    QString get_unit(const FormID &id) const;
    /// \endcond
    // clang-format off
/*! \fn string get_unit(string data_engine_field);
    \brief Returns the unit of the data field
    \param data_engine_field The datafield-ID
    \returns unit property of datafield

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local unit = data_engine:get_unit("measurements/current")
    --unit contains: "mA"

    \endcode
With the \link desired_value_database desired value file \endlink:
     \code{.json}
{
    "measurements":{
        "title":"Measurements",
        "data":[
            {	"name": "current",
                "value": 100,
                "unit": "mA",
                "tolerance": "+3/-9",
                "si_prefix": 1e-3,
                "nice_name":    "Suppy Current"}
        ]
    }
}
    \endcode
*/



#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    string_table get_section_names();
#endif
    /// \cond HIDDEN_SYMBOLS
    QStringList get_section_names() const;
    /// \endcond
    // clang-format off
/*! \fn string_table get_section_names();
    \brief Returns all section names of the data_engine

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local result = data_engine:get_section_names()
    --result= {"measurements","general_information"}

    \endcode
With the \link desired_value_database desired value file \endlink:
     \code{.json}
{
    "measurements":{
        "title":"Measured values",
        "data":[
            {	"name": "current",
                "value": 100,
                "unit": "mA",
                "tolerance": "+3/-9",
                "si_prefix": 1e-3,
                "nice_name":    "Suppy Current"}
        ]
    },
    "general_information":{
        "title":"Measurements",
        "data":[
            {	"name": "current",
                "value": 100,
                "unit": "mA",
                "tolerance": "+3/-9",
                "si_prefix": 1e-3,
                "nice_name":    "Suppy Current"}
        ]
    }
}
    \endcode
*/



#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    string_table get_ids_of_section(string section_name);
#endif
    /// \cond HIDDEN_SYMBOLS
    struct Sol_table get_ids_of_section(sol::state *lua, const std::string &section_name);
    /// \endcond
    // clang-format off
/*! \fn     string_table get_ids_of_section(string section_name);
    \brief Returns all data field ids of the specified section
    \param section_name specifying the section name

     \par examples:
     \code{.lua}
    local SAVE_FILEPATH =
        DATA_ENGINE_AUTO_DUMP_PATH..
        "test_reports\\data_engine_and_report_1\\"..
        tostring(device_serial_number).."\\"

    local dependency_tags = {}
    local data_engine = Data_engine.new("report_template.lrxml",
            "desire_values_aka_data_engine_source.json",
            SAVE_FILEPATH,dependency_tags)

    local result = data_engine:get_ids_of_section("measurements")
    --result= {"measurements/current_a","measurements/current_b"}

    \endcode
With the \link desired_value_database desired value file \endlink:
     \code{.json}
{
    "measurements":{
        "title":"Measured values",
        "data":[
            {	"name": "current_a",
                "value": 100,
                "unit": "mA",
                "tolerance": "+3/-9",
                "si_prefix": 1e-3,
                "nice_name":    "Suppy Current"},
            {	"name": "current_b",
                "value": 100,
                "unit": "mA",
                "tolerance": "+3/-9",
                "si_prefix": 1e-3,
                "nice_name":    "Suppy Current"}
        ]
    }
}
    \endcode
*/


#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    save_to_json(string filename);
#endif
    /// \cond HIDDEN_SYMBOLS
    void save_to_json(QString filename);
    /// \endcond
    // clang-format off
/*! \fn save_to_json(string filename);
    \brief Saves the current state of the data_engine to a json file. This is an extra function since after the script
finishes such a data dump file is generated anyways.
    \param filename The filename of the data dump file
    \sa data_engine_dump_file_format
*/





    /// \cond HIDDEN_SYMBOLS

    QString get_actual_dummy_value(const FormID &id) const;
    EntryType get_entry_type(const FormID &id) const;
    double get_si_prefix(const FormID &id) const;
    QString get_section_title(const QString section_name) const;
    bool is_desired_value_set(const FormID &id) const;


    Sol_table get_section_names(sol::state *lua);
    bool section_uses_variants(QString section_name) const;
    QStringList get_instance_captions(const QString &section_name) const;


    QStringList get_ids_of_section(const QString &section_name) const;

    void do_exceptional_approvals(ExceptionalApprovalDB &ea_db, QWidget *parent);
    void fill_engine_with_dummy_data();
    Statistics get_statistics() const;

    std::unique_ptr<QWidget> get_preview() const;
    bool generate_pdf(const std::string &form, const std::string &destination) const;
    void fill_database(QSqlDatabase &db) const;
    void generate_template(const QString &destination, const QString &db_filename, QString report_title, QString image_footer_path, QString image_header_path,
                           QString approved_by_field_id, QString static_text_report_header, QString static_text_page_header, QString static_text_page_footer,
                           QString static_text_report_footer_above_signature, QString static_text_report_footer_beneath_signature,
                           const QList<PrintOrderItem> &print_order) const;

    static void replace_database_filename(const std::string &source_form_path, const std::string &destination_form_path, const std::string &database_path);

    bool section_uses_instances(QString section_name) const;

    void set_enable_auto_open_pdf(bool auto_open_pdf);

    bool do_exceptional_approval(ExceptionalApprovalDB &ea_db, QString field_id, QWidget *parent);

    EntryType get_entry_type_dummy_mode(const FormID &id) const;
    /// \endcond
    private:
    QString get_actual_value_raw(const FormID &id) const;
    void generate_pages(QXmlStreamWriter &xml, QString report_title, QString image_footer_path, QString image_header_path, QString approved_by_field_id,
                        QString static_text_report_header, QString static_text_page_header, QString static_text_page_footer,
                        QString static_text_report_footer_above_signature, QString static_text_report_footer_beneath_signature,
                        const QList<PrintOrderItem> &print_order) const;
    void generate_pages_header(QXmlStreamWriter &xml, QString report_title, QString image_footer_path, QString image_header_path, QString approved_by_field_id,
                               QString static_text_report_header, QString static_text_page_header, QString static_text_page_footer,
                               QString static_text_report_footer_above_signature, QString static_text_report_footer_beneath_signature,
                               const QList<PrintOrderItem> &print_order) const;
    void generate_tables(const QList<PrintOrderItem> &print_order) const;
    void generate_sourced_form(const std::string &source_path, const std::string &destination_path, const std::string &database_path) const;
    void add_sources_to_form(QString data_base_path, const QList<PrintOrderItem> &print_order, QString approved_by_field_id) const;

    struct FormIdWrapper {
        FormIdWrapper(const FormID &id)
            : value(id) {}
        FormIdWrapper(const std::unique_ptr<DataEngineDataEntry> &entry)
            : value(entry->get_description()) {}
        FormIdWrapper(const std::pair<FormID, std::unique_ptr<DataEngineDataEntry>> &entry)
            : value(entry.first) {}
        QString value;
    };

    void assert_not_in_dummy_mode() const;
    static bool entry_compare(const FormIdWrapper &lhs, const FormIdWrapper &rhs);

    DataEngineActualValueStatisticFile statistics_file;
    DataEngineSections sections;
    QString script_path;
    quint64 load_time_seconds_since_epoch = 0;
    int generate_image(QXmlStreamWriter &xml, QString image_path, int y_position, QString parent_name) const;
    void generate_table(const DataEngineSection *section) const;
    QList<PrintOrderSectionItem> get_print_order(const QList<PrintOrderItem> &orig_print_order, bool used_for_textfields,
                                                 TextFieldDataBandPlace actual_band_position) const;
    int generate_textfields(QXmlStreamWriter &xml, int y_start, const QList<PrintOrderItem> &print_order, TextFieldDataBandPlace actual_band_position) const;
    bool auto_open_pdf = false;
    void generate_exception_approval_table() const;
    void do_exceptional_approval_(ExceptionalApprovalDB &ea_db, QList<FailedField> failed_fields, QWidget *parent);
    int generate_static_text_field(QXmlStreamWriter &xml, int y_start, const QString static_text, TextFieldDataBandPlace actual_band_position) const;
    void assert_in_dummy_mode() const;
    std::unique_ptr<Communication_logger> logger;
    /// \endcond
};
/** \} */ // end of group data_engine
#endif    // DATA_ENGINE_H
