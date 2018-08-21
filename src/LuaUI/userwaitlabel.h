#ifndef USERWAITLABEL_H
#define USERWAITLABEL_H
#include "scriptengine.h"
#include "ui_container.h"
#include <QMetaObject>
#include <QString>
#include <functional>
#include <string>

class QLabel;
class QSplitter;
class QTimer;
class QCheckBox;
class QHBoxLayout;
class QWidget;
class QLineEdit;
class QPushButton;
class QPushButton;

class UserWaitLabel : public UI_widget {
    public:
    UserWaitLabel(UI_container *parent, ScriptEngine *script_engine, std::string instruction_text);
    ~UserWaitLabel();
    void set_text(const std::string &instruction_text);
    void set_visible(bool visible);

    void set_enabled(bool enabled);

    void scale_columns();

    private:
    QLabel *label_user_instruction = nullptr;
    QLabel *spinner_label = nullptr;
    QTimer *timer = nullptr;



    QHBoxLayout *hlayout = nullptr;
    QString instruction_text;
    uint blink_state = 0;
    bool run_hotkey_loop();

    ///\cond HIDDEN_SYMBOLS
    void start_timer();
    ///\endcond

    bool is_question_mode = false;

    QMetaObject::Connection callback_timer = {};

    ScriptEngine *script_engine;
    int total_width = 10;
    void resizeMe(QResizeEvent *event) override;
    bool is_init = false;
};

#endif // USERINSTRUCTIONLABEL_H
