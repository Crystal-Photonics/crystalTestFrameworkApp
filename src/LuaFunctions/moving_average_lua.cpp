#include "moving_average_lua.h"
#include "moving_average.h"
#include "scriptengine.h"
#include "scriptsetup_helper.h"

void bind_moving_average(sol::state &lua, QPlainTextEdit *console) {
    lua.new_usertype<MovingAverage>("MovingAverage", sol::meta_function::construct, sol::factories([console = console](double average_time_ms) {
                                        abort_check();
                                        return MovingAverage{console, average_time_ms};
                                    }),
                                    "append", wrap(&MovingAverage::append),           //
                                    "get_average", wrap(&MovingAverage::get_average), //
                                    "get_count", wrap(&MovingAverage::get_count),     //
                                    "get_min", wrap(&MovingAverage::get_min),         //
                                    "get_max", wrap(&MovingAverage::get_max),         //
                                    "get_stddev", wrap(&MovingAverage::get_stddev),   //
                                    "clear", wrap(&MovingAverage::clear),             //
                                    "is_empty", wrap(&MovingAverage::is_empty)

    );
}
