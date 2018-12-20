device_requirements = 
{

}


function run(devices)
	print(A_PATH_AS_ENVIREONMENT) --environment variables
	print(TRUE_BOOLEAN_ENVIRONMENT_VARIABLE)
	print(ONE_NUMBER_ENVIRONMENT_VARIABLE)
	
	local fn = propose_unique_filename_by_datetime(".","mytestfile",".txt")
	print(fn)
	
	table_save_to_file(fn,{"test"},false)
	local fn = propose_unique_filename_by_datetime(".","mytestfile",".txt")
	print(fn)
	fn = show_file_save_dialog("Save copy",".",{"*.txt"})
	print(fn)
	
	local source_selector = Ui.IsotopeSourceSelector.new()
	local combo = Ui.ComboBox.new({"Hello", "World","Foo"})
	local checkbox = Ui.CheckBox.new("Clickme")
	local label = Ui.Label.new("Dies ist ein Test-Label")
	local edit = Ui.LineEdit.new()
	local line = Ui.HLine.new()
	local edit2 = Ui.LineEdit.new()
	local ui_image = Ui.Image.new();
	
	ui_image:load_image_file("Jellyfish.jpg")
	local done_button = Ui.Button.new("Aufnahme beenden")

	edit:set_caption("Dies ist ein Line edit:")
	
	combo:set_caption("yey:")
	combo:set_index(2) --should return world
	
	--done_button:await_click()
	print("You slected: " ..source_selector:get_selected_serial_number())
	print("activity[kBq]: " .. round(source_selector:get_selected_activity_Bq()/1000,2))
	print("combo: " .. combo:get_text())
	local checked = checkbox:get_checked()
	local t = ""
	if checked then
		t = "true"
	else 
		t = "false"
	end
	print("checkbox: " .. t)
end
