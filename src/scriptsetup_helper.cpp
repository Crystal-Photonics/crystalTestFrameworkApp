#include "scriptsetup_helper.h"

#include <QThread>
#include <sol.hpp>

void abort_check() {
	if (QThread::currentThread()->isInterruptionRequested()) {
		throw sol::error("Abort Requested");
	}
}
