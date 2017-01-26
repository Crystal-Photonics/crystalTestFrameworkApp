#ifndef QT_UTIL_H
#define QT_UTIL_H

#include <QCoreApplication>
#include <QEvent>
#include <QThread>
#include <QVariant>
#include <cassert>
#include <future>
#include <utility>

class QTabWidget;

namespace Utility {
inline QVariant make_qvariant(void *p) {
    return QVariant::fromValue(p);
}
template <class T>
T *from_qvariant(QVariant &qv) {
    return static_cast<T *>(qv.value<void *>());
}
template <class T>
T *from_qvariant(QVariant &&qv) {
    return static_cast<T *>(qv.value<void *>());
}

template <typename Fun>
void thread_call(QObject *obj, Fun &&fun); //calls fun in the thread that owns obj

template <class T, class Fun>
struct ValueSetter;

template <class Fun>
auto promised_thread_call(QObject *object, Fun &&f) -> decltype(f()); //calls f in the thread that owns obj and waits for the function to get processed

QWidget *replace_tab_widget(QTabWidget *tabs, int index, QWidget *new_widget, const QString &title);

//=== The rest of this header is just the implementation for the templates above, don't read if you are alergic to templates.

template <typename Fun>
void thread_call(QObject *obj, Fun &&fun) {
    assert(obj->thread() || (qApp && (qApp->thread() == QThread::currentThread())));
    if (obj->thread() == QThread::currentThread()) {
        return fun();
    }
    struct Event : public QEvent {
        Fun fun;
        Event(Fun &&fun)
            : QEvent(QEvent::User)
            , fun(std::forward<Fun>(fun)) {}
        ~Event() {
            fun();
        }
    };
    QCoreApplication::postEvent(obj->thread() ? obj : qApp, new Event(std::forward<Fun>(fun)));
}

template <class Fun>
struct ValueSetter<void, Fun> {
    static void set_value(





            std::promise<void>
            &p, Fun &&
            f
            ) {
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
    return future.get();
}
}

#endif // QT_UTIL_H
