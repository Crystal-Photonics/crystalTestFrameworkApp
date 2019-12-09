#include "color_lua.h"
#include "color.h"
#include "scriptsetup_helper.h"

//create an overloaded function from a list of functions. When called the overloaded function will pick one of the given functions based on arguments.
template <class ReturnType = std::false_type, class Functions_head, class... Functions_tail>
static auto overloaded_function(Functions_head &&functions_head, Functions_tail &&... functions_tail) {
    return [functions =
                std::make_tuple(std::forward<Functions_head>(functions_head), std::forward<Functions_tail>(functions_tail)...)](sol::variadic_args args) {
        std::vector<sol::object> objects;
        for (auto object : args) {
            objects.push_back(std::move(object));
        }
#define OVERLOAD_SET(function) ([](auto &&... x) { return function(std::forward<decltype(x)>(x)...); })
        if constexpr (std::is_same_v<ReturnType, std::false_type>) {
            return std::apply(OVERLOAD_SET(detail::try_call<detail::return_type_t<Functions_head>>), std::tuple_cat(std::forward_as_tuple(objects), functions));
        } else {
            return std::apply(OVERLOAD_SET(detail::try_call<ReturnType>), std::tuple_cat(std::forward_as_tuple(objects), functions));
        }
#undef OVERLOAD_SET
    };
}

void bind_color(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
    (void)script_engine;
    (void)parent;
    ui_table.new_usertype<Color>("Color", //
                                 sol::meta_function::construct, sol::no_constructor);

    ui_table["Color"] = overloaded_function(
        +[](const std::string &name) {
            abort_check();
            return Color{name};
        },
        +[](int r, int g, int b) {
            abort_check();
            return Color{r, g, b};
        }, //
        +[](int rgb) {
            abort_check();
            return Color{rgb};
        } //
    );
}
