--[[
Model - Tour time of day for other tour
Type - MNL
Authors - Siyu Li, Harish Loganathan
]]

-- require statements do not work with C++. They need to be commented. The order in which lua files are loaded must be explicitly controlled in C++. 
-- require "Logit"

--Estimated values for all betas
--Note: the betas that not estimated are fixed to zero.

local beta_ARR_2_3 = 0.0375 
local beta_ARR_2_2 = 0.0886 
local beta_ARR_2_1 = -0.291 
local beta_ARR_2_7 = 0.0505 
local beta_ARR_2_6 = -0.0517 
local beta_ARR_2_5 = -0.327 
local beta_ARR_2_4 = 0.0152 
local beta_DEP_2_8 = 0.0197 
local beta_C = 0.0 
local beta_ARR_2_8 = -0.0343 
local beta_DEP_2_1 = -0.138 
local beta_DEP_2_3 = -0.0442 
local beta_DEP_2_2 = -0.268 
local beta_DEP_2_5 = -0.565 
local beta_DEP_1_8 = 0.27
local beta_DEP_2_7 = 0.115 
local beta_DEP_2_6 = 0.102 
local beta_DEP_1_2 = -0.271 
local beta_TT1 = 0.0
local beta_TT2 = 0.0 
local beta_DEP_1_6 = -0.71 
local beta_DEP_1_7 = -0.0667 
local beta_DEP_1_4 = -0.226 
local beta_DEP_1_5 = 0.245 
local beta_ARR_1_8 = 0.0614 
local beta_DEP_1_3 = -0.687 
local beta_DEP_1_1 = -0.0708 
local beta_ARR_1_4 = 0.227
local beta_ARR_1_5 = -2.17 
local beta_ARR_1_6 = 0.00352 
local beta_ARR_1_7 = 0.259
local beta_ARR_1_1 = -1.23 
local beta_ARR_1_2 = -1.74 
local beta_ARR_1_3 = -0.341 
local beta_DUR_1 = 0.0591
local beta_DUR_2 = -0.0723 
local beta_DUR_3 = 0.00338 
local beta_DEP_2_4 = -0.0264


local k = 3
local n = 4
local ps = 3
local pi = math.pi

local Begin={}
local End={}
local choiceset={}
local arrmidpoint = {}
local depmidpoint = {}

for i =1,48 do
	Begin[i] = i
	End[i] = i
	arrmidpoint[i] = i * 0.5 + 2.75
	depmidpoint[i] = i * 0.5 + 2.75
end

for i = 1,1176 do
	choiceset[i] = i
end

local comb = {}
local count = 0

for i=1,48 do
	for j=1,48 do
		if j>=i then
			count=count+1
			comb[count]={i,j}
		end
	end
end

local function sarr_1(t)
	return beta_ARR_1_1 * math.sin(2*pi*t/24.) + beta_ARR_1_4 * math.cos(2*pi*t/24.)+beta_ARR_1_2 * math.sin(4*pi*t/24.) + beta_ARR_1_5 * math.cos(4*pi*t/24.)+beta_ARR_1_3 * math.sin(6*pi*t/24.) + beta_ARR_1_6 * math.cos(6*pi*t/24.)
end

local function sdep_1(t)
	return beta_DEP_1_1 * math.sin(2*pi*t/24.) + beta_DEP_1_4 * math.cos(2*pi*t/24.)+beta_DEP_1_2 * math.sin(4*pi*t/24.) + beta_DEP_1_5 * math.cos(4*pi*t/24.)+beta_DEP_1_3 * math.sin(6*pi*t/24.) + beta_DEP_1_6 * math.cos(6*pi*t/24.)
end

local function sarr_2(t)
	return beta_ARR_2_1 * math.sin(2*pi*t/24.) + beta_ARR_2_4 * math.cos(2*pi*t/24.)+beta_ARR_2_2 * math.sin(4*pi*t/24.) + beta_ARR_2_5 * math.cos(4*pi*t/24.)+beta_ARR_2_3 * math.sin(6*pi*t/24.) + beta_ARR_2_6 * math.cos(6*pi*t/24.)
end

local function sdep_2(t)
	return beta_DEP_2_1 * math.sin(2*pi*t/24.) + beta_DEP_2_4 * math.cos(2*pi*t/24.)+beta_DEP_2_2 * math.sin(4*pi*t/24.) + beta_DEP_2_5 * math.cos(4*pi*t/24.)+beta_DEP_2_3 * math.sin(6*pi*t/24.) + beta_DEP_2_6 * math.cos(6*pi*t/24.)
end

local utility = {}
local function computeUtilities(params,dbparams) 
	--local person_type_id = params.person_type_id 
	-- gender in this model is the same as female_dummy
	local gender = params.female_dummy
	-- work time flexibility 1 for fixed hour, 2 for flexible hour
	--local worktime = params.worktime	
	
	local pow = math.pow

	local cost_HT1_am = dbparams.cost_HT1_am
	local cost_HT1_pm = dbparams.cost_HT1_pm
	local cost_HT1_op = dbparams.cost_HT1_op
	local cost_HT2_am = dbparams.cost_HT2_am
	local cost_HT2_pm = dbparams.cost_HT2_pm
	local cost_HT2_op = dbparams.cost_HT2_op

	for i = 1,1176 do
		local arrid = comb[i][1]
		local depid = comb[i][2]
		local arr = arrmidpoint[arrid]
		local dep = depmidpoint[depid]
		local dur = dep - arr
		local arr_am = 0
		local arr_pm = 0
		local arr_op = 0
		local dep_am = 0
		local dep_pm = 0
		local dep_op = 0

		if arr<9.5 and arr>7.5 then
			arr_am, arr_pm, arr_op = 1, 0, 0
		elseif arr < 19.5 and arr > 17.5 then
			arr_am, arr_pm, arr_op = 0, 1, 0
		else
			arr_am, arr_pm, arr_op = 0, 0, 1
		end

		if dep <9.5 and dep >7.5 then 
			dep_am, dep_pm, dep_op = 1, 0, 0
		elseif dep<19.5 and dep > 17.5 then 
			dep_am, dep_pm, dep_op = 0, 1, 0
		else
			dep_am, dep_pm, dep_op = 0, 0, 1
		end
		utility[i] = sarr_1(arr) + sdep_1(dep) + gender * (sarr_2(arr) + sdep_2(dep)) + beta_DUR_1 * dur + beta_DUR_2 * pow(dur,2) + beta_DUR_3 * pow(dur,3) + beta_TT1 * dbparams:TT_HT1(arrid) + beta_TT2 * dbparams:TT_HT2(depid) + beta_C * (cost_HT1_am * arr_am + cost_HT1_pm * arr_pm + cost_HT1_op * arr_op + cost_HT2_am * dep_am + cost_HT2_pm * dep_pm + cost_HT2_op * dep_op)
	end
end

--availability
--the logic to determine availability is the same with current implementation
local availability = {}
local function computeAvailabilities(params,dbparams)
	for i = 1, 1176 do 
		availability[i] = params:getTimeWindowAvailabilityTour(i)
	end
end

--scale
local scale={}
for i = 1, 1176 do
	scale[i]=1
end

function choose_ttdo(params,dbparams)
	computeUtilities(params,dbparams) 
	computeAvailabilities(params,dbparams)
	local probability = calculate_probability("mnl", choiceset, utility, availability, scale)
	return make_final_choice(probability)
end
