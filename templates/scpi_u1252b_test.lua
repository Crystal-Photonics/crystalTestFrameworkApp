protocols = {"SCPI"}
deviceNames = {"U1252B"}

--require "gamma_finder_library.lua"


	
function SCPI_acceptable(device_descriptor)
    local expectedGitHash = "44a9f18"
	local dd = device_descriptor
	if dd.Name == "U1252B" then
			return nil
		else
			--show_warning("Incorrect hash", dd.hash_out)
			--for key,value in pairs(dd) do print(key,value) end
			return "Incorrect Hash: Got " .. dd.GitHash .. " expected: " .. expectedGitHash
	end
end

function run(devices)
	local u1252b  = devices[1]
	local result = u1252b:get_str("*IDN");
	print(result)
	local plot = Ui.Plot.new()
	local curve1 = plot:add_curve()
	local curve2 = plot:add_curve()
	print("serialnumber",u1252b:get_serial_number())
	print("get_name",u1252b:get_name())
	print("get_manufacturer",u1252b:get_manufacturer())
	curve1:set_color(Ui.Color_from_name("black"))
	curve2:set_color(Ui.Color_from_name("blue"))
	local done_button = Ui.Button.new("Aufnahme beenden")
	--sleep_ms(1000)
	i = 1;
	while not done_button:has_been_pressed() do
		if true then
			result_events = u1252b:get_event_list();
			if #result_events > 0 then
				print(result_events)
			end
			if u1252b:is_event_received("*L") then
				print("key pressed")
			end
		end
		u1252b:clear_event_list();
		
		if true then
			--local result1 = u1252b:get_str("FETC")
			--local result2 = u1252b:get_str_param("FETC","@2")
			local result3 = u1252b:get_num("FETC")
			local result4 = u1252b:get_num_param("FETC","@2")
			curve1:append_point(i,result3)
			curve2:append_point(i,result4)
			print(result3 , "    ", result4 );
		end
		--u1252b:send_command("SYST:BEEP TONE")
		--sleep_ms(200)
		i=i+1
	end
end
