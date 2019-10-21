#ifndef USERINSTRUCTIONLABEL_H
#define USERINSTRUCTIONLABEL_H
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
/** \ingroup ui
 *  \{
 */
class UserInstructionLabel : public UI_widget {
public:
  UserInstructionLabel(UI_container *parent, ScriptEngine *script_engine,
                       std::string instruction_text);
  ~UserInstructionLabel();
  void set_instruction_text(const std::string &instruction_text);
  void set_visible(bool visible);

  void set_enabled(bool enabled);
  void await_event();
  bool await_yes_no();

  void scale_columns();

private:
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
  uint blink_state = 0;
  bool run_hotkey_loop();

  ///\cond HIDDEN_SYMBOLS
  void start_timer();
  ///\endcond

  bool is_question_mode = false;

  QMetaObject::Connection callback_timer = {};
  QMetaObject::Connection callback_button_yes = {};
  QMetaObject::Connection callback_button_no = {};
  QMetaObject::Connection callback_button_next = {};
  ScriptEngine *script_engine;
  int total_width = 10;
  void resizeMe(QResizeEvent *event) override;
  bool is_init = false;
};
/** \} */ // end of group ui
#endif    // USERINSTRUCTIONLABEL_H
