#ifndef USERINSTRUCTIONLABEL_H
#define USERINSTRUCTIONLABEL_H
#include <QMetaObject>
#include <QString>
#include <functional>
#include <string>
#include <QEventLoop>

class QLabel;
class QSplitter;
class QTimer;
class QCheckBox;
class QHBoxLayout;
class UI_container;
class QWidget;
class QLineEdit;
class QPushButton;
class QPushButton;
class UserInstructionLabel {
    enum { confirm_pressed, skip_pressed, cancel_pressed };

    public:
    UserInstructionLabel(UI_container *parent, std::string instruction_text);
    ~UserInstructionLabel();
    void set_instruction_text(const std::string &instruction_text);
    void set_visible(bool visible);

    void set_enabled(bool enabled);
    void await_event();
    bool await_yes_no();

    private:
    UI_container *parent;

    QLabel *label_user_instruction = nullptr;
    QTimer *timer = nullptr;

    QPushButton *button_next = nullptr;
    QLabel *label_next = nullptr;

    QPushButton *button_yes = nullptr;
    QLabel *label_yes = nullptr;

    QPushButton *button_no = nullptr;
    QLabel *label_no = nullptr;

    QHBoxLayout *hlayout = nullptr;
    QString instruction_text;
    bool blink_state = false;
    bool run_hotkey_loop();

    QEventLoop event_loop;
    ///\cond HIDDEN_SYMBOLS
    void start_timer();
    ///\endcond

    bool is_question_mode = false;
    QMetaObject::Connection callback_timer = {};
    QMetaObject::Connection callback_button_yes = {};
    QMetaObject::Connection callback_button_no = {};
    QMetaObject::Connection callback_button_next = {};
};

#endif // USERINSTRUCTIONLABEL_H
