device_requirements = 
{

}


function run(devices)
	local date_line_edit = Ui.LineEdit.new()
	local btn = Ui.Button.new("next")
	date_line_edit:set_date(os.time())
	btn:await_click()
	print(date_line_edit:get_date())
	local date_value = date_line_edit:get_date()
	print(os.date("%d.%m.%Y", date_value))
	print(date_line_edit:get_text())
end
