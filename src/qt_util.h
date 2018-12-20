#ifndef QT_UTIL_H
#define QT_UTIL_H

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QSplitter>
#include <QThread>
#include <QVariant>
#include <cassert>
#include <functional>
#include <future>
#include <utility>

#include "scriptengine.h"
class QTabWidget;

namespace Utility {
    inline QVariant make_qvariant(void *p) {
        return QVariant::fromValue(p);
    }

	QFrame *add_handle(QSplitter *splitter);

    template <class T>
    T *from_qvariant(QVariant &qv) {
        return static_cast<T *>(qv.value<void *>());
    }
    template <class T>
    T *from_qvariant(QVariant &&qv) {
        return static_cast<T *>(qv.value<void *>());
    }

    template <typename Fun>
	void thread_call(QObject *obj, Fun &&fun, ScriptEngine *script_engine_to_terminate_on_exception = nullptr); //calls fun in the thread that owns obj

    template <class T, class Fun>
    struct ValueSetter;

    template <class Fun>
	auto promised_thread_call(QObject *object, Fun &&f) -> decltype(f()); //calls f in the thread that owns obj and waits for the function to get processed

    QWidget *replace_tab_widget(QTabWidget *tabs, int index, QWidget *new_widget, const QString &title);

    class Event_filter : public QObject {
        Q_OBJECT
        public:
        Event_filter(QObject *parent);
        Event_filter(QObject *parent, std::function<bool(QEvent *)> function);
        void add_callback(std::function<bool(QEvent *)> function);
        void clear();
        bool eventFilter(QObject *object, QEvent *ev) override;

        private:
        std::vector<std::function<bool(QEvent *)>> callbacks;
    };

    /*************************************************************************************************************************
	   The rest of this header is just the implementation for the templates above, don't read if you are alergic to templates.
	 *************************************************************************************************************************/

    template <typename Fun>
	void thread_call(QObject *obj, Fun &&fun, ScriptEngine *script_engine_to_terminate_on_exception) {
        assert(obj->thread() || (qApp && (qApp->thread() == QThread::currentThread())));
        if (obj->thread() == QThread::currentThread()) {
            return fun();
        }
        // qDebug() << obj->thread();
        if (obj->thread() == nullptr) {
            qDebug() << "isRunning" << obj->thread()->isRunning();
        }
		if (obj->thread() != nullptr && not obj->thread()->isRunning()) {
			//if the target thread is dead, do not pass it messages
			throw std::runtime_error("Attempted to make a thread-call to a dead thread");
		}

        struct Event : public QEvent {
            ScriptEngine *script_engine_to_terminate_on_exception__ = nullptr;
            Fun fun;
            Event(Fun &&fun, ScriptEngine *script_engine_to_terminate_on_exception_)
                : QEvent(QEvent::User)
                , script_engine_to_terminate_on_exception__{script_engine_to_terminate_on_exception_}
                , fun(std::forward<Fun>(fun)) {}
            ~Event() {
                try {
                    fun();
                } catch (const std::exception &e) {
                    //qDebug() << e.what();
                    if (script_engine_to_terminate_on_exception__) {
                        script_engine_to_terminate_on_exception__->interrupt(QString::fromStdString(e.what()));
                        //assert(!"Must not leak exceptions to qt message queue");
                        //std::terminate();
                    } else {
                        assert(!"Must not leak exceptions to qt message queue");
                        std::terminate();
                    }
                }
            }
        };
        QCoreApplication::postEvent(obj->thread() ? obj : qApp, new Event(std::forward<Fun>(fun), script_engine_to_terminate_on_exception));
    }

    template <class Fun>
    struct ValueSetter<void, Fun> {
        static void set_value(std::promise<void> &p, Fun &&f) {
            std::forward<Fun>(f)();
            p.set_value();
        }
    };
    template <class T, class Fun>
    struct ValueSetter {
        static void set_value(std::promise<T> &p, Fun &&f) {
            p.set_value(std::forward<Fun>(f)());
        }
    };

    template <class Fun>
    auto promised_thread_call(QObject *object, Fun &&f) -> decltype(f()) {
        std::promise<decltype(f())> promise;
        auto future = promise.get_future();
		thread_call(object, [&f, &promise] {
            try {
                Utility::ValueSetter<decltype(f()), Fun>::set_value(promise, std::forward<Fun>(f));
            } catch (...) {
                promise.set_exception(std::current_exception());
            }
        });
		while (future.wait_for(std::chrono::milliseconds{16}) == std::future_status::timeout) {
			QApplication::processEvents();
		}
        return future.get();
    }
}

#endif // QT_UTIL_H
