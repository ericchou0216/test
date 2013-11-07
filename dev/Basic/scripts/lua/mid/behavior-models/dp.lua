--[[
Model - Day Pattern
Type - MNL
Authors - Siyu Li, Harish Loganathan
]]

-- all require statements do not work with C++. They need to be commented. The order in which lua files are loaded must be explicitly controlled in C++. 
--require "Logit"

--Estimated values for all betas
--Note: the betas that not estimated are fixed to zero.
--Tour constants
beta_tour_work = -6.34
beta_tour_edu = 0
4beta_tour_shop = -3.54
beta_tour_others = -4.00

--Intermediate Stops constants
beta_stop_work = 0
beta_stop_edu = 0
beta_stop_shop = 0
beta_stop_others = 0

--Person type
beta_parttime_work = -0.300
beta_parttime_edu = 0
beta_parttime_shop = 0.0938
beta_parttime_others = 0.630
beta_selfemployed_work = -1.55
beta_selfemployed_edu = 0
beta_selfemployed_shop = -0.121
beta_selfemployed_others = 0.803
beta_universitystudent_work = -2.31
beta_universitystudent_edu = 0
beta_universitystudent_shop = 0
beta_universitystudent_others = 0
beta_homemaker_work = 0
beta_homemaker_edu = 0
beta_homemaker_shop = 0.753
beta_homemaker_others = 1.11
beta_retired_work = 0
beta_retired_edu = 0
beta_retired_shop = 0.548
beta_retired_others = 1.23
beta_unemployed_work = 0
beta_unemployed_edu = 0
beta_unemployed_shop = 0.475
beta_unemployed_others = 1.64
beta_nationalservice_work = 0.494
beta_nationalservice_edu = 0
beta_nationalservice_shop = 0
beta_nationalservice_others = -0.270
beta_voluntary_work = -1.18
5beta_voluntary_edu = 0
beta_voluntary_shop = 0.177
beta_voluntary_others = 0
beta_domestic_work= 0
beta_domestic_edu = 0
beta_domestic_shop = -1.39
beta_domestic_others = -0.391
beta_otherworker_work = -1.49
beta_otherworker_edu = 0
beta_otherworker_shop = -3.98
beta_otherworker_others = -3.12
beta_student16_work = -2.24
beta_student16_edu = 0
beta_student16_shop = -0.736
beta_student16_others = -0.199
beta_student515_work = 0
beta_student515_edu = 0
beta_student515_shop = -1.33
beta_student515_others = -0.191
beta_child4_work = 0
beta_child4_edu = 0
beta_child4_shop = 0
beta_child4_others = 0

--Adult age group
beta_age2025_work = 0
beta_age2025_edu = 0
beta_age2025_shop = 0
beta_age2025_others = 0
beta_age2635_work = 0
beta_age2635_edu = 0
beta_age2635_shop = 0
beta_age2635_others = 0
beta_age5165_work = 0
beta_age5165_edu = 0
beta_age5165_shop = 0
beta_age5165_others = 0

--Adult gender/children
beta_maleage4_work = 0
beta_maleage4_edu = 0
beta_maleage4_shop = -0.718
beta_maleage4_others = 0.454
beta_maleage515_work = 0
beta_maleage515_edu = 0
beta_maleage515_shop = -0.674
beta_maleage515_others = 0.575
beta_femalenone_work = -0.236
beta_femalenone_edu = 0
beta_femalenone_shop = 0
beta_femalenone_others = -0.366
beta_femaleage4_work = -1.02
beta_femaleage4_edu = 0
beta_femaleage4_shop = -0.379
beta_femaleage4_others = 0.219
beta_femaleage515_work = 0
beta_femaleage515_edu = 0
beta_femaleage515_shop = -0.161
beta_femaleage515_others = 0.289

--Household composition
beta_onlyadults_work = 0
beta_onlyadults_edu = 0
beta_onlyadults_shop = -0.498
beta_onlyadults_others = 0.325
beta_onlyworkers_work = 0
beta_onlyworkers_edu = 0
beta_onlyworkers_shop = 0.172
beta_onlyworkers_others = 0.321

--Personal income
beta_income_work = 0.000268
beta_income_edu = 0
beta_income_shop = -0.000208
beta_income_others = 0.000104

--Others
beta_workathome_work = 0
beta_workathome_edu = 0
beta_workathome_shop = 0
beta_workathome_others = 0
beta_caravail_work = -0.902
beta_caravail_edu = 0
beta_caravail_shop = -0.0947
beta_caravail_others = 0.623
beta_motoravail_work = 0.465
beta_motoravail_edu = 0
beta_motoravail_shop = -0.231
beta_motoravail_others = 0.0117

--Additional constants
beta_onetour_onestop = -2.66
beta_onetour_twostop = -4.45
beta_onetour_threestop = 0
beta_twotour_onestop = -2.91
beta_twotour_twostop = 0
beta_twotour_threestop = 0
beta_threetour_onestop = 0
beta_threetour_twostop = 0

beta_work_logsum=0.596
beta_edu_logsum=0
beta_shopping_logsum=0.0675
beta_other_logsum=0.146

--choiceset
choice = {
        {0,0,0,0,0,0,0,0}, 
        {0,0,0,1,0,0,0,0}, 
        {0,0,0,1,0,0,0,1}, 
        {0,0,0,1,0,0,1,0}, 
        {0,0,0,1,0,0,1,1}, 
        {0,0,1,0,0,0,0,0}, 
        {0,0,1,0,0,0,0,1}, 
        {0,0,1,0,0,0,1,0}, 
        {0,0,1,0,0,0,1,1}, 
        {0,0,1,1,0,0,0,0}, 
        {0,0,1,1,0,0,0,1}, 
        {0,0,1,1,0,0,1,0}, 
        {0,1,0,0,0,0,0,0}, 
        {0,1,0,0,0,0,0,1}, 
        {0,1,0,0,0,0,1,0}, 
        {0,1,0,0,0,0,1,1}, 
        {0,1,0,0,0,1,0,0}, 
        {0,1,0,0,0,1,0,1}, 
        {0,1,0,0,0,1,1,0}, 
        {0,1,0,1,0,0,0,0}, 
        {0,1,0,1,0,0,0,1}, 
        {0,1,0,1,0,0,1,0}, 
        {0,1,0,1,0,1,0,0}, 
        {0,1,1,0,0,0,0,0}, 
        {0,1,1,0,0,0,0,1}, 
        {0,1,1,0,0,0,1,0}, 
        {0,1,1,0,0,1,0,0}, 
        {0,1,1,1,0,0,0,0}, 
        {1,0,0,0,0,0,0,0}, 
        {1,0,0,0,0,0,0,1}, 
        {1,0,0,0,0,0,1,0}, 
        {1,0,0,0,0,0,1,1}, 
        {1,0,0,0,1,0,0,0}, 
        {1,0,0,0,1,0,0,1}, 
        {1,0,0,0,1,0,1,0}, 
        {1,0,0,1,0,0,0,0}, 
        {1,0,0,1,0,0,0,1}, 
        {1,0,0,1,0,0,1,0}, 
        {1,0,0,1,1,0,0,0}, 
        {1,0,1,0,0,0,0,0}, 
        {1,0,1,0,0,0,0,1}, 
        {1,0,1,0,0,0,1,0}, 
        {1,0,1,0,1,0,0,0}, 
        {1,0,1,1,0,0,0,0}, 
        {1,1,0,0,0,0,0,0}, 
        {1,1,0,0,0,0,0,1}, 
        {1,1,0,0,0,0,1,0}, 
        {1,1,0,0,0,1,0,0}, 
        {1,1,0,0,1,0,0,0}, 
        {1,1,0,1,0,0,0,0}, 
        {1,1,1,0,0,0,0,0}
}

--fixed variables (it is sufficient to compute these variables just once for the whole simulation)
fixedVariablesComputed = false -- initialized to false

  -- WorkTi,EduTi,ShopTi,OthersTi, WorkIi,EduIi,ShopIi,OthersIi
WorkT = {}
EduT = {}
ShopT = {}
OthersT = {}
WorkI = {}
EduI = {}
ShopI = {}
OthersI = {}
	
  -- XXXtour_XXXstop. . . series
onetour_onestop = {}
onetour_twostop = {}
onetour_threestop = {}
twotour_onestop = {}
twotour_twostop = {}
twotour_threestop = {}
threetour_onestop = {}
threetour_twostop = {}

  -- worki,edui,shopi,otheri series 
work = {}
edu = {}
shop = {}
other = {}

function loadFixedVariables()
	for i = 0,50 do 
		-- WorkTi,EduTi,ShopTi,OthersTi, WorkIi,EduIi,ShopIi,OthersIi
		WorkT[i] = choice[i][0]
		EduT[i] = choice[i][1]
		ShopT[i] = choice[i][2]
		OthersT[i] = choice[i][3]
		WorkI[i] = choice[i][4]
		EduI[i] = choice[i][5]
		ShopI[i] = choice[i][6]
		OthersI[i] = choice[i][7]
	
		-- XXXtour_XXXstop. . . series
		onetour_onestop[i] = 0
		onetour_twostop[i] = 0
		onetour_threestop[i] = 0
		twotour_onestop[i] = 0
		twotour_twostop[i] = 0
		twotour_threestop[i] = 0
		threetour_onestop[i] = 0
		threetour_twostop[i] = 0
		local totT = WorkT[i] + EduT[i] + ShopT[i] + OthersT[i]
		local totI = WorkI[i] + EduI[i] + ShopI[i] + OthersI[i]
		if totT == 1 then
			if totI == 1 then 
				onetour_onestop[i] = 1
			elseif totI == 2 then 
				onetour_twostop[i] = 1
			elseif totI == 3 then
				onetour_threestop[i] = 1
			end
		elseif totT == 2 then
			if totI == 1 then 
				twotour_onestop[i] = 1
			elseif totI == 2 then 
				twotour_twostop[i] = 1
			elseif totI == 3 then
				twotour_threestop[i] = 1
			end
		elseif totT == 3 then
			if totI == 1 then 
				threetour_onestop[i] = 1
			elseif totI == 2 then 
				threetour_twostop[i] = 1
			end
		end
		
		-- worki,edui,shopi,otheri series 
		work[i],edu[i],shop[i],other[i] = 0,0,0,0
		if (WorkT[i] + WorkI[i]) >= 1 then
			work[i] = 1
		elseif (EduT[i] + EduI[i]) >= 1 then
			edu[i] = 1
		elseif (ShopT[i] + ShopI[i]) >= 1 then
			shop[i] = 1
		elseif (OthersT[i] + OthersI[i]) >= 1 then
			other[i] = 1
		end
	end
	fixedVariablesComputed = true	
end

--utility
utility = {}
function computeUtilities(params) 
	-- storing data from params table passed into this function locally for use in this function (this is purely for better execution time)
	local person_type_id = params.person_type_id
	local age_id = params.age_id
	local universitystudent = params.universitystudent
	local only_adults = params.only_adults
	local only_workers = params.only_workers 
	local num_underfour = params.num_underfour
	local presence_of_under15 = params.presence_of_under15
	local female_dummy = params.female_dummy
	local HH_with_under_4 = params.HH_with_under_4
	local HH_with_under_15 = params.HH_with_under_15
	local HH_all_adults = params.HH_all_adults
	local income_id = params.income_id
	local income_mid = {500,1250,1750,2250,2750,3500,4500,5500,6500,7500,8500,0,99999,99999}
	local work_at_home_dummy = params. work_at_home_dummy
	local car_own_normal = params.car_own_normal
	local car_own_offpeak = params.car_own_offpeak
	local motor_own = params.motor_own
	local worklogsum = params.worklogsum
	local edulogsum = params.edulogsum
	local shoplogsum = params.shoplogsum
	local otherlogsums = params.otherlogsum

	-- person type related variables
	local fulltime,parttime,selfemployed,universitystudent,homemaker,retired,unemployed,nationalservice,voluntary,domestic,otherworker,student16,student515,child4 = 0,0,0,0,0,0,0,0,0,0,0,0,0,0
	if person_type_id == 1 then
		fulltime = 1
	elseif person_type_id == 2 then 
		parttime = 1
	elseif person_type_id == 3 then
		selfemployed = 1
	elseif person_type_id == 4 and universitystudent == 1 then 
		universitystudent = 1
	elseif person_type_id == 5 then
		homemaker = 1
	elseif person_type_id == 6 then
		retired = 1
	elseif person_type_id == 7 then
		unemployed = 1
	elseif person_type_id == 8 then
		nationalservice = 1
	elseif person_type_id == 9 then
		voluntary = 1
	elseif person_type_id == 12 then
		otherworker = 1
	end 
	if person_type_id == 4 and age_id == 3 then 
		student16 = 1
	elseif person_type_id == 4 and (age_id == 1 or age_id == 2) then
		student515 = 1
	end
	if age_id == 0 then child4 = 1 end
	
	-- age group related variables
	local age20,age2025,age2635,age3650,age5165,age65 = 0,0,0,0,0,0
	if age_id < 4 then 
		age20 = 1
	elseif age_id == 4 then 
		age2025 = 1
	elseif age_id == 5 or age_id == 6 then 
		age2635 = 1
	elseif age_id == 7 or age_id == 8 or age_id == 9 then 
		age3650 = 1
	elseif age_id == 10 or age_id == 11 or age_id == 12 then 
		age5165 = 1
	elseif age_id > 12 then 
		age65 = 1
	end

	-- Household Composition related variables
	local onlyadults,onlyworkers,HH_with_under_4,HH_with_under_15 = 0,0,0,0
	if only_adults == 1 then onlyadults = 1 end
	if only_workers == 1 then onlyworkers = 1 end
	if num_underfour >= 1 then HH_with_under_4 = 1 end
	if presence_of_under15 >= 1 then HH_with_under_15 = 1 end

	-- Adult gender/children related variables
	local maleage4,maleage515,malenone,femalenone,femaleage4,femaleage515 = 0,0,0,0,0,0
	if female_dummy == 0 and HH_with_under_4 == 1 then maleage4 = 1 end
	if female_dummy == 0 and HH_with_under_4 == 0 and HH_with_under_15 == 1 then maleage515 = 1 end
	if female_dummy == 0 and HH_all_adults == 1 then malenone = 1 end
	if female_dummy == 1 and HH_with_under_4 == 1 then femaleage4 = 1 end
	if female_dummy == 1 and HH_with_under_4 == 0 and HH_with_under_15 == 1 then femaleage515 = 1 end
	if female_dummy == 1 and HH_all_adults == 1 then femalenone = 1 end
	
	-- income related variables
	local income,missingincome = 0,0
	if income_id>=13 then missingincome = 1 end
	income = income_mid[income_id] * (1 - missingincome)

	-- other variables
	local workathome,caravail,motoravail = 0,0,0
	if work_at_home_dummy == 1 then workathome = 1 end
	if car_own_normal == 1 or car_own_offpeak == 1 then caravail = 1 end
	if motor_own == 1 then motoravail = 1 end
	
	utility[0] = 0
	for i = 1,50 do
		utility[i] = 
			beta_tour_work * WorkT[i] + beta_stop_work * WorkI[i] +
			beta_tour_edu * EduT[i] + beta_stop_edu * EduI[i] +
			beta_tour_shop * ShopT[i] + beta_stop_shop * ShopI[i] +
			beta_tour_others * OthersT[i] + beta_stop_others * OthersI[i] +
			beta_onetour_onestop * onetour_onestop[i] +
			beta_onetour_twostop * onetour_twostop[i] +
			beta_onetour_threestop * onetour_threestop[i] +
			beta_twotour_onestop * twotour_onestop[i] +
			beta_twotour_twostop * twotour_twostop[i] +
			beta_twotour_threestop * twotour_threestop[i] +
			beta_threetour_onestop * threetour_onestop[i] +
			beta_threetour_twostop * threetour_twostop[i] +
			beta_parttime_work * (work[i] * parttime) +
			beta_parttime_edu * (edu[i] * parttime) +
			beta_parttime_shop * (shop[i] * parttime) +
			beta_parttime_others * (other[i] * parttime) +
			beta_selfemployed_work * (work[i] * selfemployed) +
			beta_selfemployed_edu * (edu[i] * selfemployed) +
			beta_selfemployed_shop * (shop[i] * selfemployed) +
			beta_selfemployed_others * (other[i] * selfemployed) +
			beta_universitystudent_work * (work[i] * universitystudent) +
			beta_universitystudent_edu * (edu[i] * universitystudent) +
			beta_universitystudent_shop * (shop[i] * universitystudent) +
			beta_universitystudent_others * (other[i] * universitystudent) +
			beta_homemaker_work * (work[i] * homemaker) +
			beta_homemaker_edu * (edu[i] * homemaker) +
			beta_homemaker_shop * (shop[i] * homemaker) +
			beta_homemaker_others * (other[i] * homemaker) +
			beta_retired_work * (work[i] * retired) +
			beta_retired_edu * (edu[i] * retired) +
			beta_retired_shop * (shop[i] * retired) +
			beta_retired_others * (other[i] * retired) +
			beta_unemployed_work * (work[i] * unemployed) +
			beta_unemployed_edu * (edu[i] * unemployed) +
			beta_unemployed_shop * (shop[i] * unemployed) +
			beta_unemployed_others * (other[i] * unemployed) +
			beta_nationalservice_work * (work[i] * nationalservice) +
			beta_nationalservice_edu * (edu[i] * nationalservice) +
			beta_nationalservice_shop * (shop[i] * nationalservice) +
			beta_nationalservice_others * (other[i] * nationalservice) +
			beta_voluntary_work * (work[i] * voluntary) +
			beta_voluntary_edu * (edu[i] * voluntary) +
			beta_voluntary_shop * (shop[i] * voluntary) +
			beta_voluntary_others * (other[i] * voluntary) +
			beta_domestic_work * (work[i] * domestic) +
			beta_domestic_edu * (edu[i] * domestic) +
			beta_domestic_shop * (shop[i] * domestic) +
			beta_domestic_others * (other[i] * domestic) +
			beta_otherworker_work * (work[i] * otherworker) +
			beta_otherworker_edu * (edu[i] * otherworker) +
			beta_otherworker_shop * (shop[i] * otherworker) +
			beta_otherworker_others * (other[i] * otherworker) +
			beta_student16_work * (work[i] * student16) +
			beta_student16_edu * (edu[i] * student16) +
			beta_student16_shop * (shop[i] * student16) +
			beta_student16_others * (other[i] * student16) +
			beta_student515_work * (work[i] * student515) +
			beta_student515_edu * (edu[i] * student515) +
			beta_student515_shop * (shop[i] * student515) +
			beta_student515_others * (other[i] * student515) +
			beta_child4_work * (work[i] * child4) +
			beta_child4_edu * (edu[i] * child4) +
			beta_child4_shop * (shop[i] * child4) +
			beta_child4_others * (other[i] * child4) +
			beta_age2025_work * (work[i] * age2025) +
			beta_age2025_edu * (edu[i] * age2025) +
			beta_age2025_shop * (shop[i] * age2025) +
			beta_age2025_others * (other[i] * age2025) +
			beta_age2635_work * (work[i] * age2635) +
			beta_age2635_edu * (edu[i] * age2635) +
			beta_age2635_shop * (shop[i] * age2635) +
			beta_age2635_others * (other[i] * age2635) +
			beta_age5165_work * (work[i] * age5165) +
			beta_age5165_edu * (edu[i] * age5165) +
			beta_age5165_shop * (shop[i] * age5165) +
			beta_age5165_others * (other[i] * age5165) +
			beta_maleage4_work * (work[i] * maleage4) +
			beta_maleage4_edu * (edu[i] * maleage4) +
			beta_maleage4_shop * (shop[i] * maleage4) +
			beta_maleage4_others * (other[i] * maleage4) +
			beta_maleage515_work * (work[i] * maleage515) +
			beta_maleage515_edu * (edu[i] * maleage515) +
			beta_maleage515_shop * (shop[i] * maleage515) +
			beta_maleage515_others * (other[i] * maleage515) +
			beta_femalenone_work * (work[i] * femalenone) +
			beta_femalenone_edu * (edu[i] * femalenone) +
			beta_femalenone_shop * (shop[i] * femalenone) +
			beta_femalenone_others * (other[i] * femalenone) +
			beta_femaleage4_work * (work[i] * femaleage4) +
			beta_femaleage4_edu * (edu[i] * femaleage4) +
			beta_femaleage4_shop * (shop[i] * femaleage4) +
			beta_femaleage4_others * (other[i] * femaleage4) +
			beta_femaleage515_work * (work[i] * femaleage515) +
			beta_femaleage515_edu * (edu[i] * femaleage515) +
			beta_femaleage515_shop * (shop[i] * femaleage515) +
			beta_femaleage515_others * (other[i] * femaleage515) +
			beta_onlyadults_work * (work[i] * onlyadults) +
			beta_onlyadults_edu * (edu[i] * onlyadults) +
			beta_onlyadults_shop * (shop[i] * onlyadults) +
			beta_onlyadults_others * (other[i] * onlyadults) +
			beta_onlyworkers_work * (work[i] * onlyworkers) +
			beta_onlyworkers_edu * (edu[i] * onlyworkers) +
			beta_onlyworkers_shop * (shop[i] * onlyworkers) +
			beta_onlyworkers_others * (other[i] * onlyworkers) +
			beta_income_work * (work[i] * income) +
			beta_income_edu * (edu[i] * income) +
			beta_income_shop * (shop[i] * income) +
			beta_income_others * (other[i] * income) +
			beta_workathome_work * (work[i] * workathome) +
			beta_workathome_edu * (edu[i] * workathome) +
			beta_workathome_shop * (shop[i] * workathome) +
			beta_workathome_others * (other[i] * workathome) +
			beta_caravail_work * (work[i] * caravail) +
			beta_caravail_edu * (edu[i] * caravail) +
			beta_caravail_shop * (shop[i] * caravail) +
			beta_caravail_others * (other[i] * caravail) +
			beta_motoravail_work * (work[i] * motoravail) +
			beta_motoravail_edu * (edu[i] * motoravail) +
			beta_motoravail_shop * (shop[i] * motoravail) +
			beta_motoravail_others * (other[i] * motoravail) +
			beta_work_logsum * WorkT[i] * worklogsum +
			beta_edu_logsum * EduT[i] * edulogsum +
			beta_shopping_logsum * ShopT[i] * shoplogsum +
			beta_other_logsum * OthersT[i] * otherlogsum
	end
end

--availability
availability = {}
function computeAvailabilities(params)
	-- storing data from params table passed into this function locally for use in this function (this is purely for better execution time)
	local person_type_id = params.person_type_id

	for i = 0,50 do
		-- For Full time student (person_type_id=4): All alternatives are available.
		-- For other person type: only alternatives with EduT=0 (i.e. choice[i][1] = 0) are available to them
		if person_type_id == 4 then
			availability[i] = 1
		else
			availability[i] = 0
			if choice[i][1] == 0 then 
				availability[i] = 1
			end
		end
	end
end

-- scales
scale = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1} -- 1 for all choices (51 1s)

function choose(params)
	if not fixedVariablesComputed then loadFixedVariables() end
	computeUtilities(params) 
	computeAvailabilities(params)
	local probability = calculate_probability("mnl", choice, utility, availability, scale)
	return make_final_choice(probability)
end



