device_requirements = 
{
	{
		protocol = "SCPI",
		device_names = {"HM8150"},
		quantity = 1
	}
}

	
function SCPI_acceptable(device_descriptor)
    local expectedGitHash = "44a9f18"
	local dd = device_descriptor
	if dd.Name == "HM8150" then
			return nil
		else
			--show_warning("Incorrect hash", dd.hash_out)
			--for key,value in pairs(dd) do print(key,value) end
			return "Incorrect Hash: Got " .. dd.GitHash .. " expected: " .. expectedGitHash
	end
end

function run(devices)
	local hm8150  = devices[1]
	local result = hm8150:get_str("*IDN");
	print(result)

	print("serialnumber",hm8150:get_serial_number())
	print("get_name",hm8150:get_name())
	print("get_manufacturer",hm8150:get_manufacturer())
	hm8150:send_command("FRQ:1000") --Frequenz auf Dispaly anzeigen
	print("frequenz: " .. hm8150:get_num("FRQ") .. "Hz")
	hm8150:send_command("DFR") --Frequenz auf Dispaly anzeigen
	sleep_ms(100)
	hm8150:send_command("FRQ:2.3E3") --Frequenz auf Dispaly anzeigen
	sleep_ms(100)
	hm8150:send_command("DAM") --Amplitude auf Dispaly anzeigen
	sleep_ms(100)
	hm8150:send_command("DOF") --Amplitude auf Dispaly anzeigen
	sleep_ms(100)
	hm8150:send_command("FRQ:1.3E4") --Frequenz auf Dispaly anzeigen
	print("aktuelle frequenz: " .. hm8150:get_num("FRQ") .. "Hz")
end
