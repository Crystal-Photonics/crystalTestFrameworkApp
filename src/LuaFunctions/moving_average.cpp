#include "moving_average.h"
#include "Windows/mainwindow.h"
#include "console.h"
#include "qt_util.h"

#include <QDateTime>

#include <sol.hpp>

MovingAverage::MovingAverage(QPlainTextEdit *console, double average_time_ms) {
    this->console = console;
    this->m_average_time_ms = average_time_ms;
}

MovingAverage::~MovingAverage() {}

void MovingAverage::append(double value) {
    m_average_storage[QDateTime::currentMSecsSinceEpoch()] = value;
    auto keys = m_average_storage.uniqueKeys();
    for (auto k : keys) {
        if (k < QDateTime::currentMSecsSinceEpoch() - m_average_time_ms) {
            m_average_storage.remove(k);
        }
    }
}

double MovingAverage::get_average() {
    const auto &values = m_average_storage.values();
    if (values.count() == 0) {
        return 0;
    }
    auto sum = std::accumulate(values.begin(), values.end(), 0.0);
    auto mean = sum / values.count();
    return mean;
}

double MovingAverage::get_stddev() {
    const auto &values = m_average_storage.values();
    if (values.count() == 0) {
        return -1;
    }
    auto sq_sum = std::inner_product(values.begin(), values.end(), values.begin(), 0.0);
    auto sum = std::accumulate(values.begin(), values.end(), 0.0);
    auto mean = sum / values.count();
    auto stdev = std::sqrt(sq_sum / values.size() - mean * mean);
    return stdev;
}

double MovingAverage::get_count() {
    return m_average_storage.values().count();
}

double MovingAverage::get_min() {
    const auto &values = m_average_storage.values();
    return *std::min_element(values.begin(), values.end());
}

double MovingAverage::get_max() {
    const auto &values = m_average_storage.values();
    return *std::max_element(values.begin(), values.end());
}

void MovingAverage::clear() {
    m_average_storage.clear();
}

bool MovingAverage::is_empty() {
    return m_average_storage.values().count() == 0;
}
