#ifndef UTILITY_H
#define UTILITY_H

#include <QString>
#include <chrono>
#include <experimental/optional>
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
				std::forward<InitFunction>(init)();
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
	} // namespace impl
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
	using Optional = std::experimental::optional<T>;

	template <class T>
	typename std::add_const<T>::type &as_const(T &t) {
		return t;
	}

	template <class... Args>
	struct Overload_picker {
		template <class Return_type, class Class>
		constexpr auto operator()(Return_type (Class::*function_pointer)(Args...)) const {
			return function_pointer;
		}
		template <class Return_type, class Class>
		constexpr auto operator()(Return_type (Class::*function_pointer)(Args...) const) const {
			return function_pointer;
		}
	};

	template <class... Args>
	constexpr Overload_picker<Args...> pick_overload = {};
} // namespace Utility

#ifdef __GNUC__
#define PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define PRETTY_FUNCTION __FUNCSIG__
#else
#define PRETTY_FUNCTION __FUNCTION__
#endif

#define LOG()                                                                                                                                                  \
	auto log_printer = Utility::RAII_do([now = std::chrono::high_resolution_clock::now(), function = PRETTY_FUNCTION] {                                        \
		static std::chrono::nanoseconds sum;                                                                                                                   \
		const auto diff = std::chrono::high_resolution_clock::now() - now;                                                                                     \
		sum += diff;                                                                                                                                           \
		qDebug() << function << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() << '/'                                                     \
				 << std::chrono::duration_cast<std::chrono::milliseconds>(sum).count() << "ms";                                                                \
	})

#endif // UTILITY_H
