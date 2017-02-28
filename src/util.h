#ifndef UTILITY_H
#define UTILITY_H

#include <QString>
#include <sstream>
#include <type_traits>
#include <utility>

namespace Utility {
	template <class T>
	bool convert(const std::wstring &ws, T &t) {
		return !(std::wistringstream(ws) >> t).fail();
	}
	template <class T>
	bool convert(const std::string &s, T &t) {
		return !(std::istringstream(s) >> t).fail();
	}
	template <class T>
	bool convert(const QString &qs, T &t) {
		return convert(qs.toStdWString(), t);
	}

	namespace impl {
		template <typename F>
		struct RAII_Helper {
			template <typename InitFunction>
			RAII_Helper(InitFunction &&init, F &&exit)
				: f_(std::forward<F>(exit))
				, canceled(false) {
				init();
			}
			RAII_Helper(F &&f)
				: f_(f)
				, canceled(false) {}
			~RAII_Helper() {
				if (!canceled) {
					f_();
				}
			}
			void cancel() {
				canceled = true;
			}

			private:
			F f_;
			bool canceled;
		};
	}
	template <class F>
	impl::RAII_Helper<F> RAII_do(F &&f) {
		return impl::RAII_Helper<F>(std::forward<F>(f));
	}

	template <class Init, class Exit>
	impl::RAII_Helper<Exit> RAII_do(Init &&init, Exit &&exit) {
		return impl::RAII_Helper<Exit>(std::forward<Init>(init), std::forward<Exit>(exit));
	}

	QString to_human_readable_binary_data(const QByteArray &data);
	QString to_human_readable_binary_data(const QString &data);
	QString to_C_hex_encoding(const QByteArray &data);

	template <class T>
	struct alignas(std::alignment_of<T>()) Optional {
		Optional()
			: valid(false) {}
		Optional(T &&t)
			: valid(true) {
			new (data) T(t);
		}
		Optional(Optional &&other)
			: valid(other.valid) {
			if (valid) {
				new (data) T(std::move(other.get_value()));
			}
		}
		~Optional() {
			if (valid) {
				get_value().~T();
			}
		}

		bool has_value() const {
			return valid;
		}
		T &get_value() & {
			if (!has_value()) {
				throw std::runtime_error("Tried to get value from empty Optional");
			}
			return *reinterpret_cast<T *>(data);
		}
		T &&get_value() && {
			if (!has_value()) {
				throw std::runtime_error("Tried to get value from empty Optional");
			}
			return std::move(*reinterpret_cast<T *>(data));
		}
		const T &get_value() const & {
			if (!has_value()) {
				throw std::runtime_error("Tried to get value from empty Optional");
			}
			return *reinterpret_cast<const T *>(data);
		}

		private:
		char data[sizeof(T)];
		bool valid{false};
	};
}

#endif // UTILITY_H
