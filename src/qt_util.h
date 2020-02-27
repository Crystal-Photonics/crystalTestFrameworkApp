#ifndef QT_UTIL_H
#define QT_UTIL_H

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QEvent>
#include <QFrame>
#include <QSplitter>
#include <QThread>
#include <QVariant>
#include <cassert>
#include <functional>
#include <future>
#include <utility>

class ScriptEngine;
class QTabWidget;

void interrupt_script_engine(ScriptEngine *script_engine, QString message = {});
bool currently_in_gui_thread(); //defined in mainwindow.cpp, declared in mainwindow.h, but can't include because of circular dependencies

namespace Utility {
    QFrame *add_handle(QSplitter *splitter);

    //calls fun in the thread that owns obj and returns immediately
    template <typename Fun>
    void thread_call(QObject *obj, Fun &&fun, ScriptEngine *script_engine_to_terminate_on_exception = nullptr);

    template <class T, class Fun>
    struct ValueSetter;

    //calls f in the thread that owns object, waits for the function to get processed and returns the result
    template <class Fun>
    auto promised_thread_call(QObject *object, Fun &&f) -> decltype(f());

    //calls function in the thread that owns object, returns immediately and then calls continuation in the calling thread
    template <class Function, class Continuation>
    void thread_call_then(QObject *object, Function &&function, Continuation &&continuation) {
        thread_call(object, [f = std::forward<Function>(function), continuation_object = std::make_unique<QObject>(),
                             continuation = std::forward<Continuation>(continuation)]() mutable {
            QObject *cont_obj = continuation_object.get();
            if constexpr (std::is_same_v<decltype(f()), void>) {
                f();
                thread_call(cont_obj,
                            [continuation = std::move(continuation), continuation_object = std::move(continuation_object)]() mutable { continuation(); });
            } else {
                thread_call(cont_obj, [continuation = std::move(continuation), continuation_object = std::move(continuation_object), argument = f()]() mutable {
                    continuation(argument);
                });
            }
        });
    }

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

    template <class T>
    T async_get(std::future<T> &&future) {
        //FIXME: put assert in and fix all the places it activates using thread_call_then
        //assert(not currently_in_gui_thread()); //we can't have the gui thread spin here because then the next spin will block this spin
        while (future.wait_for(std::chrono::milliseconds{16}) == std::future_status::timeout) {
            if (QThread::currentThread()->isInterruptionRequested()) {
                throw std::runtime_error{"Interrupted"};
            }
            QApplication::processEvents();
        }
        return future.get();
    }

    /*************************************************************************************************************************
       The rest of this header is just the implementation for the templates above, don't read if you are alergic to templates.
     *************************************************************************************************************************/

    template <typename Fun>
    void thread_call(QObject *obj, Fun &&fun, ScriptEngine *script_engine_to_terminate_on_exception) {
        if (not obj->thread()) {
            if (not qApp || (qApp->thread() != QThread::currentThread())) {
                throw std::runtime_error{"Internal error: Trying to thread_call on object not associated with a thread outside of main thread"};
            }
        }
        if (obj->thread() == QThread::currentThread()) {
            fun();
            return;
        }
        if (obj->thread() != nullptr && not obj->thread()->isRunning()) {
            //if the target thread is dead, do not pass it messages
            throw std::runtime_error("Attempted to make a thread-call to a dead thread");
        }

        struct Event : public QEvent {
            ScriptEngine *script_engine_to_terminate_on_exception__ = nullptr;
            Fun fun;
            Event(Fun fun, ScriptEngine *script_engine_to_terminate_on_exception_)
                : QEvent(QEvent::User)
                , script_engine_to_terminate_on_exception__{script_engine_to_terminate_on_exception_}
                , fun{std::move(fun)} {}
            ~Event() {
                try {
                    fun();
                } catch (const std::exception &e) {
                    //qDebug() << e.what();
                    if (script_engine_to_terminate_on_exception__) {
                        interrupt_script_engine(script_engine_to_terminate_on_exception__, QString::fromStdString(e.what()));
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
    auto promised_thread_call(QObject *object, Fun &&f) -> decltype(f()) {
        std::promise<decltype(f())> promise;
        auto future = promise.get_future();
        thread_call(object, [lf = std::forward<Fun>(f), promise = std::move(promise)]() mutable {
            try {
                if (QThread::currentThread()->isInterruptionRequested()) {
                    throw std::runtime_error{"Interrupted"};
                }
                if constexpr (std::is_same_v<decltype(f()), void>) {
                    std::move(lf)();
                    promise.set_value();
                } else {
                    promise.set_value(std::move(lf)());
                }
            } catch (...) {
                promise.set_exception(std::current_exception());
            }
        });
        //wait until f is done executing
        while (future.wait_for(std::chrono::milliseconds{16}) == std::future_status::timeout) {
            if (QThread::currentThread()->isInterruptionRequested()) {
                /* This is a bad situation.
                 * On one hand we got interrupted and should quit.
                 * On the other hand f is still running and relies on us not quitting so that references stay valid.
                 */
                auto start_wait = std::chrono::system_clock::now();
                while (future.wait_for(std::chrono::milliseconds{16}) == std::future_status::timeout) {
                    if (std::chrono::system_clock::now() - start_wait > std::chrono::seconds(10)) {
                        qDebug().nospace() << "******DEBUG: Been waiting on a promised_thread_call after an interrupt for 10 seconds. Possibly a deadlock.\n"
                                              "Consider putting a breakpoint in "
                                           << __FILE__ << ':' << __LINE__ << '\n';
                        start_wait += std::chrono::hours(24); //just to disable repeated printing
                    }
                    QApplication::processEvents();
                }
            }
            QApplication::processEvents();
        }
        return future.get();
    }
    struct Qt_thread : QObject {
        Q_OBJECT

        public:
        Qt_thread();
        void quit();
        void adopt(QObject &object);
        void start(QThread::Priority priority = QThread::InheritPriority);
        bool wait(unsigned long time = ULONG_MAX);
        void message_queue_join();
        void requestInterruption();
        bool was_interrupted() const;
        bool isRunning() const;
        bool is_current() const;
        QObject &qthread_object();

        private:
        QThread thread;
        signals:
        void quit_thread();
    };
} // namespace Utility

#endif // QT_UTIL_H
