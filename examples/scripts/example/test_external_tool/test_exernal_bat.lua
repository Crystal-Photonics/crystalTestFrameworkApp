device_requirements = 
{

}


function run(devices)
	local result = run_external_tool(".","jlink.bat",{},30)
	print(result)
end
