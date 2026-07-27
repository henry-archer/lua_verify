--[[
  Test suite for Lua Schema Validator (test_validator.lua)
--]]

local validator = require("validator")

local passed = 0
local failed = 0

local function assert_test(name, condition, msg)
    if condition then
        print("[PASS] " .. name)
        passed = passed + 1
    else
        print("[FAIL] " .. name .. ": " .. (msg or "assertion failed"))
        failed = failed + 1
    end
end

print("=== Running Lua Schema Validator Tests ===\n")

-- Test 1: Basic Valid Configuration
local schema_1 = {
    title = { type = "string", required = true },
    iterations = { type = "number", min = 1, max = 1000, required = true },
    debug = { type = "boolean", required = false }
}

local config_1 = {
    title = "Thermal Convection Simulation",
    iterations = 500,
    debug = true
}

local ok, errs = validator.validate(config_1, schema_1)
assert_test("Basic Valid Config", ok == true, tostring(errs))

-- Test 2: Strict Typing (no implicit coercion)
local config_strict_type = {
    title = "Thermal Convection Simulation",
    iterations = "500" -- String instead of number
}
local ok2, errs2 = validator.validate(config_strict_type, schema_1)
assert_test("Strict Typing (string '500' rejected for number)", ok2 == false)
if errs2 then
    assert_test("Strict Typing Error Message", errs2[1]:find("expected type 'number', got 'string'") ~= nil, errs2[1])
end

-- Test 3: Boundary Min / Max Limits
local config_out_of_bounds = {
    title = "Boundary Test",
    iterations = 2000 -- Exceeds max 1000
}
local ok3, errs3 = validator.validate(config_out_of_bounds, schema_1)
assert_test("Max Boundary Enforcement", ok3 == false)
if errs3 then
    assert_test("Max Boundary Error Message", errs3[1]:find("is greater than maximum allowed value 1000") ~= nil, errs3[1])
end

-- Test 4: Enum Validation
local schema_enum = {
    solver = { type = "string", enum = {"cg", "gmres", "bicgstab"}, required = true }
}
local config_enum_ok = { solver = "gmres" }
local config_enum_bad = { solver = "jacobi" }

local ok_e1, _ = validator.validate(config_enum_ok, schema_enum)
assert_test("Valid Enum Value", ok_e1 == true)

local ok_e2, errs_e2 = validator.validate(config_enum_bad, schema_enum)
assert_test("Invalid Enum Value Rejected", ok_e2 == false)
if errs_e2 then
    assert_test("Enum Error Message", errs_e2[1]:find("is not in allowed list") ~= nil, errs_e2[1])
end

-- Test 5: Optional Parent Table Omitted (Passes)
local schema_nested = {
    solver = {
        type = "table",
        required = false, -- Optional Parent
        schema = {
            method = { type = "string", required = true }, -- Required Child
            tolerance = { type = "number", required = true, min = 1e-12, max = 1e-2 }
        }
    }
}

local config_parent_omitted = {} -- Solver omitted entirely
local ok_p1, errs_p1 = validator.validate(config_parent_omitted, schema_nested)
assert_test("Optional Parent Omitted (Passes)", ok_p1 == true, tostring(errs_p1))

-- Test 6: Optional Parent Table Provided (Required Child Enforced)
local config_parent_provided_incomplete = {
    solver = {
        -- Missing required field 'method'
        tolerance = 1e-5
    }
}
local ok_p2, errs_p2 = validator.validate(config_parent_provided_incomplete, schema_nested)
assert_test("Optional Parent Provided - Missing Required Child Fails", ok_p2 == false)
if errs_p2 then
    assert_test("Nested Error Path Correct", errs_p2[1] == "Error at 'solver.method': missing required field", errs_p2[1])
end

-- Test 7: Unknown Keys & Typo Suggestion
local config_typo = {
    title = "Typo Test",
    iteratons = 100 -- Typo for 'iterations'
}
local ok_t, errs_t = validator.validate(config_typo, schema_1)
assert_test("Unknown Key Detected", ok_t == false)
if errs_t then
    assert_test("Typo Hint Suggestion", errs_t[1]:find("Did you mean 'iterations'?") ~= nil, errs_t[1])
end

-- Test 8: Error Accumulation (Multiple Errors simultaneously)
local config_multi_errors = {
    -- title is missing (Error 1)
    iterations = -50, -- Below min 1 (Error 2)
    debug = "yes", -- Wrong type (Error 3)
    extra_field = 123 -- Unknown key (Error 4)
}
local ok_m, errs_m = validator.validate(config_multi_errors, schema_1)
assert_test("Multiple Errors Detected Simultaneously", ok_m == false and #errs_m >= 4, "Got " .. tostring(errs_m and #errs_m or 0) .. " errors")

print(string.format("\n=== Test Results: %d Passed, %d Failed ===", passed, failed))
if failed > 0 then
    os.exit(1)
end
