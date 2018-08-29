device_requirements = 
{

}


 
function run(devices)

	charge_counter = ChargeCounter.new()
	
	local done_button = Ui.Button.new("Aufnahme beenden")
	

	local last_ms=0
	while not done_button:has_been_clicked() do
		charge_counter:add_current(3600);
		print("charge [Ah]: "..charge_counter:get_current_hours())
		sleep_ms(1000)
		
		print("run_time_ms: "..current_date_time_ms()-last_ms)
		last_ms = current_date_time_ms()
	end
	pc_speaker_beep();
	
end