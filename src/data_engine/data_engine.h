#ifndef DATA_ENGINE_H
#define DATA_ENGINE_H

#include <experimental/optional>
#include <istream>
#include <memory>
#include <string>
#include <vector>

class QWidget;

using FormID = std::string;

struct Numeric_entry {
	bool valid() const;

	FormID id{};
	double target_value{};
	std::experimental::optional<double> actual_value{};
	double deviation{};
	std::string unit{};
};

struct Text_entry {
	FormID id{};
	std::string target_value{};
	std::experimental::optional<std::string> actual_value{};
};

class Data_engine {
	public:
	Data_engine(std::istream &source);

	bool is_complete() const;
	bool all_values_in_range() const;
	bool value_in_range(const FormID &id) const;
	void set_actual_number(const FormID &id, double number);
	void set_actual_text(const FormID &id, std::string text);
	double get_desired_value(const FormID &id);
	double get_desired_absolute_tolerance(const FormID &id);
	double get_desired_relative_tolerance(const FormID &id);
	double get_desired_minimum(const FormID &id);
	double get_desired_maximum(const FormID &id);
	std::string &get_desired_text(const FormID &id);
	const std::string &get_unit(const FormID &id);

	std::unique_ptr<QWidget> get_preview() const;
	void generate_pdf(const std::string &path);
};

#endif // DATA_ENGINE_H
