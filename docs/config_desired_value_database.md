Desired Value Database {#desired_value_database_content}
============

# Introduction
The desired value database is the base file needed for the data engine and the report generation. Hence, the structure of the file is closely orientated towards the section structure of the report.

The goal of a desired value database is to a have a central place where all measured and acquired values can be stored to. Such a central file is much easier to overview than if each value was defined individually somewhere in the lua script. To use such a database means that you can use the data engine to automatically check whether all values are within the defined tolerances. The structure can be directly used for generating pdf reports and data dumps for being able to analyze the measurements later in the future with e.g. python scripts.

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

# File
The database file is a \glos{json} file which can be edited by a simple text editor. As the pdf report is a listing of sections, the \glos{json} top-element is an array where each element represents the section of the pdf report. Each section contains several descriptive fields and, most importantly, an array of data fields. These data fields describe desired values with tolerances and units, they store the actual measurement and they contain a descriptive text which will be displayed in the report. The identifier string “section_name/field_name” is used to access the field from the lua script.

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
			{	"name": "seriennummer",        		"type": "number",																	"nice_name":   "Seriennummer" 								},
			{	"name": "bool_test1",        		"type": "bool",																		"nice_name":   "Just a Bool test with desired value" 		},
			{	"name": "bool_test2",        		"value": true,																		"nice_name":   "Just a Bool test" 							},
			{	"name": "supply_voltage_free_mv",  	"type": "number",	"unit": "mV", 							"si_prefix": 1000, 		"nice_name":    "Freie Versorgungsspannugn mv"   			},
			{	"name": "supply_voltage_free_v",  	"type": "number",	"unit": "V", 							"si_prefix": 1, 		"nice_name":    "Freie Versorgungsspannugn V"   			},
			{	"name": "max_current_1",    		"value": 100,		"unit": "mA", 	"tolerance": "+3/-9",	"si_prefix": 1000, 		"nice_name":    "Fester Strom Toleranz 1"     	},
			{	"name": "max_current_2",    		"value": 100,		"unit": "mA", 	"tolerance": "+*/-0",	"si_prefix": 1000, 		"nice_name":    "Fester Strom Toleranz 2"     	},
			{	"name": "max_current_3",    		"value": 100,		"unit": "mA", 	"tolerance": 5,			"si_prefix": 1000, 		"nice_name":    "Fester Strom Toleranz 3"     	},
			{	"name": "max_current_4",    		"value": 100,		"unit": "mA", 	"tolerance": "10%",		"si_prefix": 1000, 		"nice_name":    "Fester Strom Toleranz 4"     	},
			{	"name": "reference_test",    		"value": "[unprinted_1/unprinted_activity.actual]",			"tolerance": "10%",		"nice_name":    "Test mit Referenz"     	}
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

## Section element
A section element contains the following field:

“title”
“instance_count”
"variants"

### variants
"apply_if"
"data":(array)of fields


## Field element

### References
 
### Tolerance String

