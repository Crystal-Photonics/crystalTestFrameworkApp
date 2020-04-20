#include "chargecounter.h"

ChargeCounter::ChargeCounter() {
    reset();
}

void ChargeCounter::add_current(const double current) {
    QDateTime now_current = QDateTime::currentDateTime();
    if (last_current_valid) {
        qint64 milliseconds_last = last_current_date_time.toMSecsSinceEpoch();
        qint64 milliseconds_now = now_current.toMSecsSinceEpoch();
        double time_diff = milliseconds_now - milliseconds_last;
        time_diff /= 1000.0; //to seconds
        time_diff /= 3600.0; //to hours

        current_hours += time_diff*last_current;
    }
    last_current = current;
    last_current_date_time = now_current;
    last_current_valid = true;
}

double ChargeCounter::get_current_hours() {
    return current_hours;
}

void ChargeCounter::reset()
{
    last_current_valid = false;
    current_hours = 0;
}
