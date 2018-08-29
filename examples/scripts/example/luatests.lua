device_requirements = 
{

}

 
function run(devices)

	
	print(tostring(8025.0))
	print(tostring(math.floor(8025.0)))
	errors = 0
	int_val = 536
	t =  {-5,-4,-3,-2,-1, 0, 1, 2, 3, 4, 5, 6}
	t2 = {-5,-4,-3,-2,-1, 1, 2, 3, 4, 5, 6}
	t3 = {-7,-4,-3,-2,-1, 1, 2, 3, 4, 5, 6}
	a =                             { 0, 1, 2, 3, 4}
	b =  {-1,-2, 2, 3, 4}
	
	t_indexed = {["A"] = -5, ["B"] = -4,["C"] = -3,["D"] = -2,["E"] = -1,["F"] = 0,["G"] = 1,["H"] = 2,["I"] = 3,["J"] = 4,["K"] = 5,["L"] = 6}

	
	table_add_table_at_result_1 =  {-5,-3,-1,1,3,0,1,2,3,4,5,6}
	table_add_table_at_result_2 =  {-5,-4,-2,0,2,4,1,2,3,4,5,6}
	table_add_table_at_result_8 =  {-5,-4,-3,-2,-1,0,1,2,4,6,8,10}
	table_add_table_at_result_10 = {-5,-4,-3,-2,-1,0,1,2,3,4,6,8,3,4}
	
	table_add_table_at_result_15 = {-5,-4,-3,-2,-1, 0, 1, 2, 3, 4, 5, 6, 0, 0, 0, 1, 2, 3, 4}
	
	local tabelle_num = {{a=5, b = 3, c = 1}, {a=3, b = 8, c = 1}}
    local tabelle_str = {{a="foo", b = 3, c = 1}, {a="bar", b = 8, c = 1},{a="xylophon", b = 8, c = 1}}

	
	local table_max_by_field_result_str = "xylophon"
	local table_max_by_field_result_num = 5
	
	local v = table_max_by_field(tabelle_str,"a")
	print(v)
	if v ~=table_max_by_field_result_str then
		print("ERROR table_max_by_field_str: =", v)
		errors = errors + 1; 
	end
	
	local v = table_max_by_field(tabelle_num,"a")
	print(v)
	if v ~=table_max_by_field_result_num then
		print("ERROR table_max_by_field_num: =", v)
		errors = errors + 1; 
	end
	

	local v = table_min_by_field({},"a")
	print(v)
	if v ~= nil then
		print("ERROR table_min_by_field_str as empty: =", v)
		errors = errors + 1; 
	end
	
	if false then
		local v = table_min_by_field(tabelle_str,"not_existing")
		print(v)
		if v ~= nil then
			print("ERROR table_min_by_field_str with wrong field: =", v)
			errors = errors + 1; 
		end
	end
	
	local table_min_by_field_result_str = "bar"
	local table_min_by_field_result_num = 3
	
	local v = table_min_by_field(tabelle_str,"a")
	print(v)
	if v ~=table_min_by_field_result_str then
		print("ERROR table_min_by_field_str: =", v)
		errors = errors + 1; 
	end
	
	
	local v = table_min_by_field(tabelle_num,"a")
	print(v)
	if v ~=table_min_by_field_result_num then
		print("ERROR table_min_by_field_num: =", v)
		errors = errors + 1; 
	end
	
	print (get_framework_git_hash())
	print (get_framework_git_date_text())
	
	v = get_framework_git_date_unix();
	print(os.date("git date: %X %x", v))
	
	v = os.time();
	print("time: " .. v)
	
	print(os.date("%X %x", v))
	 v= v+10
	print(os.date("%X %x", v))
		
	v = table_equal_table(b, a)
	if v  then
		print("ERROR unequal table_equal_table: =".. v)
		errors = errors + 1; 
	end

	v = table_equal_table(a, a)
	if v == false then
		print("ERROR equal table_equal_table: =".. v)
		errors = errors + 1; 
	end

	
	v = table_add_table_at(t, a, 1)
	if table_equal_table(v,table_add_table_at_result_1) == false then
		print("ERROR table_add_table_at 1: =", v)
		errors = errors + 1; 
	end
	
	v = table_add_table_at(t, a, 2)
	if table_equal_table(v,table_add_table_at_result_2) == false then
		print("ERROR table_add_table_at 2: =", v)
		errors = errors + 1; 
	end
	
	v = table_add_table_at(t, a, 8)
	--print(v)
	if table_equal_table(v,table_add_table_at_result_8) == false then
		print("ERROR table_add_table_at 8: =", v)
		errors = errors + 1; 
	end
	
	v = table_add_table_at(t, a, 10)
	if table_equal_table(v,table_add_table_at_result_10) == false then
		print("ERROR table_add_table_at 10: =", v)
		errors = errors + 1; 
	end
	
	v = table_add_table_at(t, a, 15)
	if table_equal_table(v,table_add_table_at_result_15) == false then
		print("ERROR table_add_table_at 15: =", v)
		errors = errors + 1; 
	end
	
	v = table_set_constant(b, 2)
	result = {2	,2	,2	,2	,2}
	if table_equal_table(v,result) == false then
		print("ERROR table_set_constant: =", v)
		errors = errors + 1; 
	end
	
	v = table_add_table(a,b)
	--print(v)
	sum_result = {-1.000000, -1.000000, 4.000000, 6.000000, 8.000000}
	if table_equal_table(v,sum_result) == false then
		print("ERROR table_add_table: =", v)
		errors = errors + 1; 
	end
	
	v = table_add_constant(a,5)
	--print(v)
	sum_result = {5, 6, 7, 8, 9.000000}
	if table_equal_table(v,sum_result) == false then
		print("ERROR table_add_constant: =", v)
		errors = errors + 1; 
	end
	

	
	v = table_sub_table(a,b)
	result = {1.000000, 3, 0.000000, 0.000000, 0.000000}
	if table_equal_table(v,result) == false then
		print("ERROR table_sub_table: =", v)
		errors = errors + 1; 
	end
	

	v = table_mul_table(a,b)
	result = {0.000000, -2, 4.000000, 9.000000, 16.000000}
	if table_equal_table(v,result) == false then
		print("ERROR table_mul_table: =", v)
		errors = errors + 1; 
	end
	
	v = table_mul_constant(a,2)
	result = {0.000000, 2, 4.000000, 6.000000, 8.000000}
	if table_equal_table(v,result) == false then
		print("ERROR table_mul_constant: =", v)
		errors = errors + 1; 
	end
	
	v = table_div_table(a,b)
	result = {0.000000, -0.5, 1.000000, 1.000000, 1.000000}
	if table_equal_table(v,result) == false then
		print("ERROR table_div_table: =", v)
		errors = errors + 1; 
	end
	
	c = {0.123456789, -0.123456789, 1.0123456789, 1.123456789, 1.128456789}
	v = table_round(c,2)
	result = {0.12, -0.12, 1.01, 1.12, 1.13}
	if table_equal_table(v,result) == false then
		print("ERROR table_round: =", v)
		errors = errors + 1; 
	end
	
	v = table_abs(b)
	result = {1	,2	,2	,3	,4}
	if table_equal_table(v,result) == false then
		print("ERROR table_abs: =", v)
		errors = errors + 1; 
	end
	
	v = table_mid(b,2,2)
	result = {-2, 2	}
	if table_equal_table(v,result) == false then
		print("ERROR table_mid: =", v)
		errors = errors + 1; 
	end
	
	v = table_mean(t)
	if v ~= 0.5 then
		print("ERROR table_mean: =".. v)
		errors = errors + 1; 
	end
		
	v = table_max(t)
	if v ~= 6 then
		print("ERROR table_max: =".. v)
		errors = errors + 1;
	end
	
	v = table_min(t)
	if v ~= -5 then
		print("ERROR table_min: =".. v)
		errors = errors + 1;
	end
	
	v = table_max_abs(t)
	if v ~= 6 then
		print("ERROR table_max_abs t: =".. v)
		errors = errors + 1;
	end	

	v = table_max_abs(t3)
	if v ~= 7 then
		print("ERROR table_max_abs t3: =".. v)
		errors = errors + 1;
	end	
	
	v = table_min_abs(t)
	if v ~= 0 then
		print("ERROR table_min_abs t: =".. v)
		errors = errors + 1;
	end		
	
	v = table_min_abs(t2)
	if v ~= 1 then
		print("ERROR table_min_abs t2: =".. v)
		errors = errors + 1;
	end	

	v = table_sum(t)
	if v ~= 6 then
		print("ERROR table_sum: =".. v)
		errors = errors + 1;
	end		
	

	v = round(1.235,0)
	if v ~= 1 then
		print("ERROR round(1.235,0): =".. v)
		errors = errors + 1;
	end		
	
	v = round(1.235,2)
	if v ~= 1.24 then
		print("ERROR round(1.235,2): =".. v)
		errors = errors + 1;
	end	

	
	print(t)
	print(t_indexed)
	print(nil)
	
	print("outputstruct: ",t_indexed) --"{1,2,3,4,5}"
    print("output int as string: "..int_val) --"{1,2,3,4,5}"
    print("output int: ",int_val) --"{1,2,3,4,5}"
	print(non_declared_variable) --nil
	print(1.1487)  -- prints "1.1487"
	
	if errors > 0 then
		print("ERRORs: ".. errors)
	else
		print("no errors")
	end
	
	print(show_question("hello", "hello world!", {"ok", "yes","no"}))
	
end