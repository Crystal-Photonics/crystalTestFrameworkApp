#ifndef TEST_DATA_ENGINE_H
#define TEST_DATA_ENGINE_H

#include "autotest.h"
#include <QObject>

class Test_Data_engine : public QObject {
    Q_OBJECT
    public:
    private slots:

    void check_tolerance_parsing_A();

    void basic_load_from_config();
    void check_properties_of_empty_set();
    void single_numeric_property_test();
    void test_text_entry();
    void test_bool_entry();
    void test_preview();
    void check_value_matching_by_name();
    void check_no_data_error_B();
    void check_no_data_error_A();
    void check_duplicate_name_error_B();
    void check_duplicate_name_error_A();
    void check_data_by_object();
    void check_wrong_tolerance_type_A();
    void check_non_existing_desired_value();
    void check_non_faulty_field_id();
    void check_non_existing_section_name();
    void check_version_string_parsing();
    void check_dependency_handling();
    void check_dependency_ambiguity_handling();
    void test_empty_entries();
    void test_references();
    void test_references_ambiguous();
    void test_references_non_existing();
    void test_references_from_non_actual_only_field();
    void test_references_string_bool();
    void test_references_set_value_in_wrong_type();
    void test_references_if_fails_when_setting_tolerance_in_bool();
    void test_references_if_fails_when_setting_tolerance_in_string();
    void test_if_fails_when_desired_number_misses_tolerance();
    void test_if_success_when_actuel_number_misses_tolerance();
    void test_references_if_fails_when_mismatch_in_unit();
    void test_references_when_number_reference_without_tolerance();
    void test_references_get_actual_value_description_desired_value();
};

DECLARE_TEST(Test_Data_engine)

#endif // TEST_DATA_ENGINE_H
