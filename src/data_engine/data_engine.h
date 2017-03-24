#ifndef DATA_ENGINE_H
#define DATA_ENGINE_H

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

struct Data_engine_entry {
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
	static std::pair<FormID, std::unique_ptr<Data_engine_entry>> from_json(const QJsonObject &object);
	virtual ~Data_engine_entry() = default;
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
	Numeric_entry(double target_value, double tolerance, QString unit, QString description);
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
	double tolerance{};
	QString unit{};
	QString description{};
	std::experimental::optional<double> actual_value{};
};

struct Text_entry : Data_engine_entry {
	Text_entry(QString target_value);

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
	double get_desired_value(const FormID &id) const;
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
	void add_entry(std::pair<FormID, std::unique_ptr<Data_engine_entry>> &&entry);
	Data_engine_entry *get_entry(const FormID &id);
	const Data_engine_entry *get_entry(const FormID &id) const;
	struct FormIdWrapper {
		FormIdWrapper(const FormID &id)
			: value(id) {}
		FormIdWrapper(const std::unique_ptr<Data_engine_entry> &entry)
			: value(entry->get_description()) {}
		FormIdWrapper(const std::pair<FormID, std::unique_ptr<Data_engine_entry>> &entry)
			: value(entry.first) {}
		QString value;
	};

	static bool entry_compare(const FormIdWrapper &lhs, const FormIdWrapper &rhs);
	std::vector<std::pair<FormID, std::unique_ptr<Data_engine_entry>>> id_entries;
	std::vector<std::unique_ptr<Data_engine_entry>> data_entries;
	void setValue(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage) const;
	void fill_report(QtRPT &report, const QString &form) const;
};

#endif // DATA_ENGINE_H
