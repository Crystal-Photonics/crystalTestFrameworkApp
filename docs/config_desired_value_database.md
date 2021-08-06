Desired Value Database {#desired_value_database_content}
============

# Introduction
The Testframework uses a concept where desired values+tolerances and acquired values are managed and checked centrally by the Data_engine. To tell the Data_engine about the data it should manage, the desired value database described in this document is used. In this file the structure, the desired values+tolerance and the to be acquired values are defined. When the script ends a report is generated based on the structure given in the *Desired Value Database* and based on the document template. Besides generating a printable document a json file is saved containing all measurements and desired values in a machine readable manner.

This means the *desired value database* is the base file needed for the data engine and the report generation. Hence, the structure of the file is closely orientated towards the section structure of the report.


## Sections 
The pdf report is organized in different sections which contain the single data fields which are defined in this database file and hold the measured or acquired actual values. Each field has 4 columns:
A descriptive text, the desired value with its tolerance, the actual measured value and an OK/fail string.
You can use different sections to separate topics. One section might list the measurement equipment, another lists basic information of the device under test (serial number, revision, firmware-version etc), another documents current consumptions etc.

## Section Variants

It is possible to define different section variants. This is useful if your product you are testing is sold in different variants. For example your device is sold with or without Bluetooth module. This would mean that its current consumption differs between both variants and you need a desired value for low and for high current consumption. With section variants you can express such differences. The data engine decides which variant to use depending the dependency_tags argument of Data_engine().

## Section instances

If it is necessary to document e.g. multiple equal accessories for the device under test(e.g. several spare batteries for the device under test), you can make use of the section instance concept. This would mean that you define a section which contains all necessary data fields for that accessory(staying at the battery example: serial number, voltage, capacity and weight). Later when the test procedure is executed you tell the data engine how many instances of the section you need(how many batteries you ship with the device under test) and let the lua script fill out each instance(each section instance documents the data of one battery).

## References

It is possible to use references between data fields. This can be used to define the desired value of field A as the actual value of field B. This method is very useful to compare values. For example if you want to ensure that the device under test measures the battery voltage correctly, you can measure the battery voltage first with a calibrated voltmeter, check in this value as the actual value of field A and place the battery voltage measurement performed by the device under test into the actual value of field B. Field B takes its desired value from A’s actual value by using references. This way you can document that the calibrated voltmeter and the device under test measure the same voltage+tolerance.

# The database File
The database file is a \glos{json} file which can be edited by a simple text editor. As the pdf report is a listing of sections, the \glos{json} top-element is a list of objects, each representing a section of the pdf report. The sections contain several descriptive fields and, most importantly, an array of data fields. These data fields describe desired values with tolerances and units, they store the actual measurement and they contain a descriptive text which will be displayed in the report. The identifier string “section_name/field_name” is used to access the field from the Lua script.

\par example:
\code
{
	"test_version":{
		"title":"Report Version",
		"data":[
			{	"name": "git_protokoll",        	"type": "string",									"nice_name":   "Git-Hash Protokoll"    						},
			{   "name": "git_framework",	 		"type": "string",      								"nice_name":   "Git-Hash Test Framework"        			},
			{	"name": "git_protokoll_date",       "type": "datetime",									"nice_name":   "Git-Datum Protokoll"    					},
			{   "name": "git_framework_date",	 	"type": "datetime",      							"nice_name":   "Git-Datum Test Framework"        			}
		]
	},
	
	"allgemein":{
		"title":"General Data",
		"data":[
			{	"name": "datum_today",        	"type": "datetime",										"nice_name":   "Datum"    						},
			{	"name": "testende_person",   	"type": "string",   									"nice_name":   "Testende Person"    			}
		]
	},
	
	"gerate_daten":{
		"title":"Device Data",
		"data":[
			{"name": "seriennummer",        	"type": "number",															  "nice_name": "Seriennummer" 					},
			{"name": "bool_test1",        		"type": "bool",																  "nice_name": "Just a Bool test with desired value"},
			{"name": "bool_test2",        		"value": true,																  "nice_name": "Just a Bool test" 				},
			{"name": "supply_voltage_free_mv",  "type": "number",	"unit": "mV", 						  "si_prefix": 1000,  "nice_name": "Freie Versorgungsspannugn mv"  	},
			{"name": "supply_voltage_free_v",  	"type": "number",	"unit": "V", 						  "si_prefix": 1, 	  "nice_name": "Freie Versorgungsspannugn V"   	},
			{"name": "max_current_1",    		"value": 100,		"unit": "mA", 	"tolerance": "+3/-9", "si_prefix": 1000,  "nice_name": "Fester Strom Toleranz 1"		},
			{"name": "max_current_2",    		"value": 100,		"unit": "mA", 	"tolerance": "+*/-0", "si_prefix": 1000,  "nice_name": "Fester Strom Toleranz 2"		},
			{"name": "max_current_3",    		"value": 100,		"unit": "mA", 	"tolerance": 5,		  "si_prefix": 1000,  "nice_name": "Fester Strom Toleranz 3"		},
			{"name": "max_current_4",    		"value": 100,		"unit": "mA", 	"tolerance": "10%",	  "si_prefix": 1000,  "nice_name": "Fester Strom Toleranz 4"		},
			{"name": "reference_test",    		"value": "[unprinted_1/unprinted_activity.actual]",		  "tolerance": "10%", "nice_name": "Test mit Referenz"				}
		]
	},
	
	"messmittel":{
		"title":"Measurement Equiment",
		"data":[
			{	"name": "multimeter_name",                    "type": "string",       "nice_name":   "Multimeter Strom Bezeichnung"                },
			{	"name": "multimeter_hersteller",              "type": "string",       "nice_name":   "Multimeter Strom Hersteller"                 },
			{	"name": "multimeter_sn",                      "type": "string",       "nice_name":   "Multimeter Strom Seriennummer"               },
			{	"name": "multimeter_calibration",             "type": "datetime",       "nice_name":   "Multimeter Strom kalibriert bis"             }
		]
	},
		
	"unprinted_1":{
		"title":"Section selected not to be printed. But will be included in the data dump.",
		"data":[
			{	"name": "unprinted_activity",    	 "type": "number",	"unit": "Bq", 			"si_prefix": 1, 	"nice_name":    "unprinted activity" },
			{   "name": "git_firmware_date_unix",	"type": "number",      											"nice_name":   	"Git Unix Date Firmware"  	}
		]
	}	
}

\endcode

## The Section element
A section element may contain the following configuration fields:

- *title*(required): Is the string displayed as the section title in the pdf report.
	\code
	{
		"test_version":{
			"title":"Report Version",
			"data":[]
		},
	}
	\endcode
<br>
- *data* (required): the data element is an array of multiple data fields. Those contain the single data points acquired by the lua script and are mostly part of the printed pdf report placed under the corresponding section. Those values can be of different types and may contain a desired value with tolerance. The data_engine checks at the end when the report is generated if all fields are completely set and whether all values are within the tolerance.
		\code
		"documentation_section":{
			"title":"Section without desired values/tolerances, for documentation purposes",
			"data":[
				{	"name": "val_1",    "type": "number",	"unit": "Bq", 			"si_prefix": 1, 	"nice_name":    "Report text 11" },
				{   "name": "val_2",	"type": "bool",      											"nice_name":   	"Report text 12" },
				{   "name": "val_3",	"type": "string",      											"nice_name":   	"Report text 13" },
				{   "name": "val_4",	"type": "datetime",     										"nice_name":   	"Report text 14" }
			]
		},
		"desired_value_section":{
			"title":"Section without desired values/tolerances, for documentation purposes",
			"data":[
				{	"name": "val_1",    "value": 2380,	"tolerance": 80, "unit": "mV",				"si_prefix": 1e-3, 	"nice_name":    "Report text 21" },
				{   "name": "val_2",	"value": "Firmware 1.4",      													"nice_name":   	"Report text 22" },
				{	"name": "val_3",    "value": true,																 	"nice_name":    "Report text 23" },
				{   "name": "val_4",	"value": "[documentation_section/val_1.actual]",  	"tolerance": "10%",			"nice_name":   	"Report text 24" },
				{   "name": "val_5",	"value": "[desired_value_section/val_1.desired]",  "tolerance": "[inherited]",	"nice_name":   	"[inherited]" 	 }
			]
		}		
		\endcode
		
		A data field contains the following elements:
	
	- *name*(required): uniquely identifies the field. Accessing the field from the Lua script follows the syntax "sectionname/fieldname".
	- *nice_name*(required): contains a descriptive text which will be printed in the pdf report besides the field values or can be used to interact with the script user.
	- *type* (required if no value): defines the type of the field if not already implicitly set by the desired value. Allowed values:
		- number
		- string
		- bool
		- datetime
		
	- *value* (required if no type is set): defines the type and the desired value of the field. Examples:
		- 10(for number):
		- "10" (for string)
		- true or false (for bool)
	
	- *unit* (required if number): string value defining the unit of the number. The unit is printed as a suffix of the number on the pdf report.
	
	- *si_prefix* (required if number): number value defining the the si-prefix of the unit. eg. 1e-3 for mV, 1 for V and 1e+3 for kV.
	
	- *tolerance* (required if number and value is set): A string which contains the tolerance of the desired number. Examples:
			| Desired Value   |      Tolerance String      |  Effective tolerance and printed in report |
			|----------|:-------------:|:------|
			| 1000.5 |	"1.5" 		| 	1000.5 (±1.5) |
			| 1000.5 |  "5%"   		|   1000.5 (±5%) |
			| 1000.5 |  "+-2"   	|   1000.5 (±2) |
			| 1000.5 |  "+5/*"   	|   ≤ 1000.5 (+5) |
			| 1000.5 |  "+0/*"   	|   ≤ 1000.5 |
			| 1000.5 |  "*/-2"   	|   ≥ 1000.5 (-2) |
			| 1000.5 |  "*/-0"   	|   ≥ 1000.5 |			
			| 1000.5 |  "+5/-2"   	|   1000.5 (+5/-2) |
			| 1000.5 |  "+5%/-2%"   |   1000.5 (+5%/-2%) |
			| 1000.5 |  "*/-2%"   	|   ≥ 1000.5 (-2%)|
			| 1000.5 |  "*"   		|   1000.5 (±∞) |
			| 1000.5 |  "*/*"   	|   1000.5 (±∞) |
			| 1000.5 |  "+*/-*"   	|   1000.5 (±∞) |
	A data field has one of the following types:
	 - *number*: a number comes always with unit, si-prefix and in case a desired value is defined with the tolerance. The data_engine evaluates the value to be correct if it is within the tolerance or if no desired value is set.
	 - *string*: The data_engine evaluates a string value to be correct if it no desired value is set or if it matches exactly.
	 - *bool*: The data_engine evaluates a bool value to be correct if it no desired value is set or if it matches exactly.
	 - *datetime*: Datetime fields don't have desired values and thus the data_engine evaluates a datetime value always to be correct.	The format printed in the report will be "yyyy-MM-dd hh:mm:ss.zzz", "yyyy-MM-dd", "hh:mm:ss", "yyyy-MM-dd hh:mm" or "yyyy-MM-dd" depending on how the date was set.
	 - *reference*: is a virtual type pointing to another field and behaves as the type of the field pointed to. A reference is defined over the *value* property using the syntax "[section/name.actual]" or "[section/name.desired]". If it points to a desired number value it is possible to inherit also the tolerance and the nice-name by using "[inherited]"
<br><br>
- *instance_count* (optional): defines how many times this section is repeated. This can be used either to reduce redundancy or to define the repetition count at runtime. e.g if you want to document multiple batteries but you don't know yet how many you are going to test while writing the test script:
	\code
	{
		"battery_test":{
			"title":"Delivered batteries",
			"instance_count":"battery_test_count",
			"data":[
				{"name": "seriennummer",    "type": "text",															 	  "nice_name": "Seriennummer" 				},
				{"name": "voltage",    		"value": 100,		"unit": "mV", 	"tolerance": "+3/-9", "si_prefix": 1000,  "nice_name": "Fester Strom Toleranz 1"	}				
			]			
		},
	}
	\endcode
 
	\sa Data_engine::use_instance() <br>
	
	And the possible Lua script:
	\code
	{
		local battery_count = 10
		data_engine:set_instance_count("battery_test_count",battery_count);
		local battery_index = 1
		for i,v in pairs(batteries) do
			data_engine:use_instance("battery_test", "Battery SN: "..v.serial_number, battery_index)
						
			data_engine:set_actual_text("battery_test/seriennummer",	v.serialnumber)		
			data_engine:set_actual_number("battery_test/voltage",		v.voltage)	
		
			battery_index = battery_index+1
		end
	}
	\endcode
	This Code will repeat the section battery_test 10 times each one titled "Battery SN: "..v.serial_number". The repetition count is defined at the script's runtime.
<br><br>
- *variants* (optional): with the variants concept it is possible to choose different sets of data during runtime. Which dataset is chosen by the data_engine is defined by the *apply_if* tag. It contains a list of conditions which all must be matching to the values of the dependency_tags Lua-table passed to the Data_engine constructor in the Lua script. An error is thrown in case of ambiguity if the condition matches to multiple datasets.
	\par Example:
	\code
	"battery_test":{
		"title":"Delivered batteries",
		"variants":[
			{
				"apply_if": {
					"celltype":"*",
					"chemistry":"nimh",
					"charger_fw_version":  "[*-1.6]",
					"big-cell": true
				},
				"data":[
					{"name": "seriennummer",    "type": "text",															 	"nice_name": "Seriennummer" 			},
					{"name": "voltage",    		"value": 1200,		"unit": "mV", 	"tolerance": "5%", "si_prefix": 1000,  	"nice_name": "NiMH Battery-voltage"}				
				]
			},{
				"apply_if": {
					"celltype":"*",
					"chemistry":"li-ion",
					"charger_fw_version":  "[*-1.6]",
					"big-cell": false					
				},
				"data":[
					{"name": "seriennummer",    "type": "text",															 	"nice_name": "Seriennummer" 			},
					{"name": "voltage",    		"value": 4200,		"unit": "mV", 	"tolerance": "5%", "si_prefix": 1000,  	"nice_name": "Li-ion Battery-voltage"}				
				]
			},{
				"apply_if": {
					"celltype":"primary",
					"chemistry":"*",
					"charger_fw_version":  "[*-1.6]",
					"big-cell": true
				},
				"data":[
					{"name": "seriennummer",    "type": "text",															 	"nice_name": "Seriennummer" 			},
					{"name": "voltage",    		"value": 1500,		"unit": "mV", 	"tolerance": "5%", "si_prefix": 1000,  	"nice_name": "Alkali Battery-voltage"}				
				]
			},{
				"apply_if": {
					"celltype":"primary",
					"chemistry":"*",
					"charger_fw_version": 	[	"[1.6-1.7]","[2.06-2.09]","2.5" ],
					"big-cell": false,
					"_comment": "selected if 1.6<=charger_fw_version<1.7 || 2.06<=charger_fw_version<2.09 || charger_fw_version == 2.5"
				},
	
				"data":[
					{"name": "seriennummer",    "type": "text",															 	"nice_name": "Seriennummer" 			},
					{"name": "voltage",    		"value": 1500,		"unit": "mV", 	"tolerance": "5%", "si_prefix": 1000,  	"nice_name": "Alkali Battery-voltage"}				
				]
			},{
				"apply_if": {
					"celltype":"primary",
					"chemistry":"*",
					"charger_fw_version": "[2.09-*]",	
					"big-cell": false,
					"_comment": "this is selected when 2.09<=charger_fw_version and type is primary"
				},
	
				"data":[
					{"name": "seriennummer",    "type": "text",															 	"nice_name": "Seriennummer" 			},
					{"name": "voltage",    		"value": 1500,		"unit": "mV", 	"tolerance": "5%", "si_prefix": 1000,  	"nice_name": "Alkali Battery-voltage"}				
				]
			}
		]
	}
	\endcode
	The corresponding Lua code can be as follows:
	\code
	local dependency_tags = {}
	dependency_tags.celltype = "primary"
	dependency_tags.chemistry = "li-ion"
	dependency_tags.charger_fw_version = 1.12
	dependency_tags.big-cell = false
	
	local data_engine = Data_engine.new("path\\to\\form.xml","path\\to\\desired_values.json","path\\where\\acquired_data\\is\\stored\\",dependency_tags)
	\endcode
	In this case the section variant with the li-ion battery would be selected.
	\par Condition definition
	Following values are allowed as condition field values:
	- string(s):
	\code
	"apply_if": {
				"text_condition_single":"green",
				"text_condition_multiple_ored": [	"yellow", "red" ]
				"text_condition_any":"*"
			}
	\endcode
		- text_condition_single matches if the corresponding dependency_tags value equals "green"
		- text_condition_multiple_ored matches if the corresponding dependency_tags value is "yellow" or "red"
		- text_condition_any matches always
	
	- number(s):
	\code
	"apply_if": {
				"number_condition_single":1.5,
				"number_condition_range": "[2-3]",	
				"number_condition_wildcat": "[3-*]",	
				"number_condition_multiple_ored": [	"[1.6-1.7]", 2.5, "2.6"]
				"number_condition_any":"*"
			}
	\endcode
		- number_condition_single matches if the corresponding dependency_tags value equals 1.5
		- number_condition_range matches if the corresponding dependency_tags value is in range 2<=value<3
		- number_condition_wildcat matches if the corresponding dependency_tags value is in range 3<=value
		- number_condition_multiple_ored matches if the corresponding dependency_tags value is in range 1.6<=value<1.7, 2.5 or 2.6
		- number_condition_any matches always
	
	- boolean:
	\code
	"apply_if": {
				"bool_condition_single_T": true,
				"bool_condition_single_F": false
			}
	\endcode
		- bool_condition_single_T matches if the corresponding dependency_tags value is true
		- bool_condition_single_F matches if the corresponding dependency_tags value is false
<br><br>
- *allow_empty_section* (optional): normally, if no matching variants can be found an error is thrown. If this *allow_empty_section* is true, the error is oppressed and an empty section is printed on the report.


