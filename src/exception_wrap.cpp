#include "exception_wrap.h"
#include "Windows/mainwindow.h"
#include "scriptengine.h"
#include "testrunner.h"
#include "ui_container.h"

QMessageBox::StandardButton ask_retry_abort_ignore(ScriptEngine *se, const std::exception &exception) {
	return Utility::promised_thread_call(se->runner->get_lua_ui_container(), [error_description = exception.what(), se] {
		return QMessageBox::critical(MainWindow::mw, QObject::tr("CrystalTestFramework - Script Error in %1").arg(se->runner->get_name()),
									 QObject::tr("The following error occured: %1\n\n").arg(error_description),
									 QMessageBox::Retry | QMessageBox::Abort | QMessageBox::Ignore);
	});
}
