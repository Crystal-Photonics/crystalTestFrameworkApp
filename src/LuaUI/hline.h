#ifndef HLINE_H
#define HLINE_H

#include "ui_container.h"

class QFrame;
class QWidget;
/** \ingroup ui
 *  \{
 */
class HLine : public UI_widget {
public:
  ///\cond HIDDEN_SYMBOLS
  HLine(UI_container *parent);
  ~HLine();
  ///\endcond

  void set_visible(bool visible);

private:
  QFrame *line = nullptr;
};
/** \} */ // end of group ui
#endif    // HLINE_H
