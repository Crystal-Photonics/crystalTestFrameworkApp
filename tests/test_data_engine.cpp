#include "test_data_engine.h"
#include "data_engine/data_engine.h"

#include <sstream>

void Test_Data_engine::load_from_config() {
	std::stringstream input{R"()"};
	Data_engine de{input};
}
