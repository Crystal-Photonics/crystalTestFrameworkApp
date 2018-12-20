device_requirements = 
{
	{
		protocol = "manual",
		device_names = {"Messscheiber"},
		quantity = 1
	}
}



--require "gamma_finder_library.lua"




function run(devices)
	local messschieber  = devices[1]
	local result = messschieber:get_name();
	print(result)
	print(messschieber:get_serial_number())
end
