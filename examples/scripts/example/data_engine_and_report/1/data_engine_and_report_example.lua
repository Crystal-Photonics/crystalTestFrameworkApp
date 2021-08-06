function run(devices)
	local device_serial_number = 2

	local test_git = {}
	if false then
		--wont work without git
		--local test_git = git_info(".",true) 
		--if test_git.modified then
		--	test_git.hash = test_git.hash .. " (nach commit geändert)"
		--end
	else
		test_git["hash"] = "0x123"
		test_git["date"] = "2018-12-19 12:50:01 +0100"
	end
	
	local SAVE_FILEPATH = DATA_ENGINE_AUTO_DUMP_PATH.."test_reports\\data_engine_and_report_1\\"..tostring(device_serial_number).."\\"
	local dependency_tags = {}
	local data_engine = Data_engine.new("report_template.lrxml", "desire_values_aka_data_engine_source.json",SAVE_FILEPATH,dependency_tags)
	data_engine:set_open_pdf_on_pdf_creation(true)
	data_engine:set_start_time_seconds_since_epoch(current_date_time_ms()/1000)

	data_engine:set_actual_text("test_version/git_protokoll",									test_git.hash )
	data_engine:set_actual_datetime_from_text("test_version/git_protokoll_date",				test_git.date )
	data_engine:set_actual_text("test_version/git_framework",									get_framework_git_hash())
	data_engine:set_actual_datetime_from_text("test_version/git_framework_date",				get_framework_git_date_text())
	
	data_engine:set_actual_datetime("allgemein/datum_today",					os.time())
	data_engine:set_actual_text("allgemein/testende_person",					get_os_username())
	
	--multimeter = devices["multimeter"]
	--multimeter = devices.multimeter
	--data_engine:set_actual_text("messmittel/multimeter_sn",						multimeter:get_serial_number())
	--data_engine:set_actual_text("messmittel/multimeter_name",						multimeter:get_name())	
	--data_engine:set_actual_text("messmittel/multimeter_hersteller",				multimeter:get_manufacturer())	
	--data_engine:set_actual_text("messmittel/multimeter_calibration",				multimeter:get_calibration())	
	
	data_engine:set_actual_text("messmittel/multimeter_sn",						"123")
	data_engine:set_actual_text("messmittel/multimeter_name",					"Multimeter")	
	data_engine:set_actual_text("messmittel/multimeter_hersteller",				"Multimeter manufacturer")	
	data_engine:set_actual_datetime_from_text("messmittel/multimeter_calibration",			"2012.01.22")	
	
	data_engine:set_actual_number("gerate_daten/seriennummer",					12568)
	data_engine:set_actual_bool("gerate_daten/bool_test1",						true)
	data_engine:set_actual_bool("gerate_daten/bool_test2",						true)
	
	data_engine:set_actual_number("gerate_daten/supply_voltage_free_mv",		100)
	data_engine:set_actual_number("gerate_daten/supply_voltage_free_v",			101)
	
	data_engine:set_actual_number("gerate_daten/max_current_1",					102/1000) --the /1000 is due to "gerate_daten/max_current_1"'s si-prefix
	data_engine:set_actual_number("gerate_daten/max_current_2",					103/1000)
	data_engine:set_actual_number("gerate_daten/max_current_3",					104/1000)
	data_engine:set_actual_number("gerate_daten/max_current_4",					105/1000)
	data_engine:set_actual_number("gerate_daten/reference_test",				106)

	data_engine:set_actual_number("unprinted_1/unprinted_activity",				107)
	data_engine:set_actual_number("unprinted_1/git_firmware_date_unix",			1547731917)	

	print(data_engine:get_actual_value("allgemein/datum_today"))
	print(data_engine:get_actual_value("test_version/git_protokoll_date"))
	print("acutal_number(gerate_daten/max_current_2): : "..tostring(data_engine:get_actual_number("gerate_daten/max_current_2")))
	print("desired value(gerate_daten/max_current_2): "..data_engine:get_desired_value("gerate_daten/max_current_2"))
	print("desired value(gerate_daten/max_current_2): "..data_engine:get_desired_value("allgemein/datum_today"))
	print("desired number(gerate_daten/max_current_2): "..tostring(data_engine:get_desired_number("gerate_daten/max_current_2")))

	
	if data_engine:all_values_in_range() then
		show_info("Habe Fertig", "Test erfolgreich abgeschlossen.")
	end
	
end

