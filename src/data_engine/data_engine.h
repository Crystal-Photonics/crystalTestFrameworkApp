#ifndef DATA_ENGINE_H
#define DATA_ENGINE_H

#include <experimental/optional>
#include <istream>
#include <memory>
#include <string>
#include <vector>

class QJsonObject;
class QWidget;
class QString;
class QVariant;

using FormID = std::string;

struct Data_engine_entry {
	void operator=(std::string text);
	void operator=(double value);
	virtual bool is_complete() const = 0;
	virtual bool is_in_range() const = 0;
	virtual QString get_display_text() const = 0;
	virtual ~Data_engine_entry() = default;
	template <class T>
	T *as();
	template <class T>
	const T *as() const;
	static std::pair<FormID, std::unique_ptr<Data_engine_entry>> from_json(const QJsonObject &object);
};

template <class T>
T *Data_engine_entry::Data_engine_entry::as() {
	return dynamic_cast<T *>(this);
}

template <class T>
const T *Data_engine_entry::Data_engine_entry::as() const {
	return dynamic_cast<const T *>(this);
}

struct Numeric_entry : Data_engine_entry {
	Numeric_entry(double target_value, double deviation, std::string unit);
	bool valid() const;
	bool is_complete() const override;
	bool is_in_range() const override;
	QString get_display_text() const override;

	double get_min_value() const;
	double get_max_value() const;

	double target_value{};
	double deviation{};
	std::string unit{};
	std::experimental::optional<double> actual_value{};
};

struct Text_entry : Data_engine_entry {
	Text_entry(std::string target_value);
	std::string target_value{};
	std::experimental::optional<std::string> actual_value{};

	bool is_complete() const override;
	bool is_in_range() const override;
	QString get_display_text() const override;
};

class Data_engine {
	public:
	Data_engine(std::istream &source);

	bool is_complete() const;
	bool all_values_in_range() const;
	bool value_in_range(const FormID &id) const;
	void set_actual_number(const FormID &id, double number);
	void set_actual_text(const FormID &id, std::string text);
	double get_desired_value(const FormID &id) const;
	double get_desired_absolute_tolerance(const FormID &id) const;
	double get_desired_relative_tolerance(const FormID &id) const;
	double get_desired_minimum(const FormID &id) const;
	double get_desired_maximum(const FormID &id) const;
	const std::string &get_desired_text(const FormID &id) const;
	const std::string &get_unit(const FormID &id) const;

	std::unique_ptr<QWidget> get_preview() const;
	void generate_pdf(const std::string &path) const;
	std::string get_json() const;

	private:
	void add_entry(std::pair<FormID, std::unique_ptr<Data_engine_entry>> &&entry);
	Data_engine_entry *get_entry(const FormID &id);
	const Data_engine_entry *get_entry(const FormID &id) const;
	struct FormIdWrapper {
		FormIdWrapper(const FormID &id)
			: value(id) {}
		FormIdWrapper(const std::pair<FormID, std::unique_ptr<Data_engine_entry>> &entry)
			: value(entry.first) {}
		const std::string &value;
	};

	static bool entry_compare(FormIdWrapper lhs, FormIdWrapper rhs);
	std::vector<std::pair<FormID, std::unique_ptr<Data_engine_entry>>> entries;
	void setValue(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage) const;
};

#endif // DATA_ENGINE_H
