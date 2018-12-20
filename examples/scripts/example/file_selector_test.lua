device_requirements = 
{

}


function run(devices)
	local file_selector = Ui.ComboBoxFileSelector.new("C:\\test\\path",{"*.lua"})
	local done_button = Ui.Button.new("Aufnahme beenden")
	file_selector:set_order_by("name",true)
	--done_button:has_been_clicked()
	done_button:await_click()
	print("You slected: " ..file_selector:get_selected_file())
end
