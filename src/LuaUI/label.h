#ifndef LABEL_H
#define LABEL_H

#include "ui_container.h"
#include <string>

class QLabel;
class QWidget;

/** \ingroup ui
 *  \{
 */
class Label : public UI_widget {
public:
  ///\cond HIDDEN_SYMBOLS
  Label(UI_container *parent, const std::string text);
  ~Label();
  ///\endcond
  void set_text(const std::string text);
  std::string get_text() const;

  void set_visible(bool visible);
  void set_enabled(bool enabled);
  void set_font_size(bool big_font);

private:
  QLabel *label = nullptr;
  int normal_font_size;
};
/** \} */ // end of group ui
#endif    // LABEL_H
