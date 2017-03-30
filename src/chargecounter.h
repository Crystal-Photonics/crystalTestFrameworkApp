#ifndef CHARGECOUNTER_H
#define CHARGECOUNTER_H
#include <QDateTime>

class ChargeCounter
{
public:
    ChargeCounter();
    void add_current(const double current);
    double get_current_hours();
    void reset();
private:
    double current_hours=0.0;
    double last_current=0;
    bool last_current_valid = false;
    QDateTime last_current_date_time;
};

#endif // CHARGECOUNTER_H
