--[[
  Test suite for Enhanced Lua Schema Validator (test_validator.lua)
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

print("=== Running Enhanced Lua Schema Validator Tests ===\n")

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
    iterations = "500"
}
local ok2, errs2 = validator.validate(config_strict_type, schema_1)
assert_test("Strict Typing (string '500' rejected for number)", ok2 == false)

-- Test 3: Boundary Min / Max Limits
local config_out_of_bounds = {
    title = "Boundary Test",
    iterations = 2000
}
local ok3, errs3 = validator.validate(config_out_of_bounds, schema_1)
assert_test("Max Boundary Enforcement", ok3 == false)

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

-- Test 5: Optional Parent Table Omitted (Passes)
local schema_nested = {
    solver = {
        type = "table",
        required = false,
        schema = {
            method = { type = "string", required = true },
            tolerance = { type = "number", required = true, min = 1e-12, max = 1e-2 }
        }
    }
}

local config_parent_omitted = {}
local ok_p1, errs_p1 = validator.validate(config_parent_omitted, schema_nested)
assert_test("Optional Parent Omitted (Passes)", ok_p1 == true, tostring(errs_p1))

-- Test 6: Optional Parent Table Provided (Required Child Enforced)
local config_parent_provided_incomplete = {
    solver = {
        tolerance = 1e-5
    }
}
local ok_p2, errs_p2 = validator.validate(config_parent_provided_incomplete, schema_nested)
assert_test("Optional Parent Provided - Missing Required Child Fails", ok_p2 == false)

-- Test 7: Unknown Keys & Typo Suggestion
local config_typo = {
    title = "Typo Test",
    iteratons = 100
}
local ok_t, errs_t = validator.validate(config_typo, schema_1)
assert_test("Unknown Key & Typo Hint", ok_t == false and errs_t[1]:find("Did you mean 'iterations'?") ~= nil)

-- Test 8: NEW FEATURE - `array_of` Rule (Scalar & Sub-table elements)
local schema_array = {
    mesh_points = {
        type = "table",
        required = true,
        array_of = { type = "number", min = 0 }
    },
    boundary_conditions = {
        type = "table",
        required = true,
        array_of = {
            type = "table",
            schema = {
                name = { type = "string", required = true },
                value = { type = "number", required = true }
            }
        }
    }
}

local config_array_valid = {
    mesh_points = {0.0, 0.5, 1.0},
    boundary_conditions = {
        { name = "inlet", value = 100.0 },
        { name = "outlet", value = 0.0 }
    }
}
local ok_arr1, errs_arr1 = validator.validate(config_array_valid, schema_array)
assert_test("Array Validation (Valid Array)", ok_arr1 == true, tostring(errs_arr1))

local config_array_invalid = {
    mesh_points = {0.0, -0.5}, -- -0.5 is below min 0
    boundary_conditions = {
        { name = "inlet" } -- missing required 'value' inside array item [1]
    }
}
local ok_arr2, errs_arr2 = validator.validate(config_array_invalid, schema_array)
assert_test("Array Validation (Invalid Array Items Caught)", ok_arr2 == false and #errs_arr2 == 2)

-- Test 9: NEW FEATURE - Custom Validator Function
local schema_custom = {
    even_number = {
        type = "number",
        required = true,
        custom = function(val, path)
            if val % 2 ~= 0 then
                return false, string.format("Error at '%s': value %d must be an even integer", path, val)
            end
            return true
        end
    }
}

local ok_c1, _ = validator.validate({ even_number = 4 }, schema_custom)
assert_test("Custom Rule (Passes Valid Value)", ok_c1 == true)

local ok_c2, errs_c2 = validator.validate({ even_number = 5 }, schema_custom)
assert_test("Custom Rule (Catches Invalid Value)", ok_c2 == false and errs_c2[1]:find("must be an even integer") ~= nil)

-- Test 10: NEW FEATURE - Auto-Documentation Generator (`to_markdown`)
local schema_doc = {
    viscosity = {
        type = "number",
        required = true,
        min = 0,
        default = 0.001,
        units = "Pa.s",
        description = "Dynamic viscosity of the fluid"
    }
}

local md_doc = validator.to_markdown(schema_doc, "Test Docs")
assert_test("Documentation Generator (`to_markdown`)", md_doc:find("| `viscosity` | `number` | **Yes** | `[0, ∞]` | `0.001` | `Pa.s` | Dynamic viscosity of the fluid |", 1, true) ~= nil)

print(string.format("\n=== Test Results: %d Passed, %d Failed ===", passed, failed))
if failed > 0 then
    os.exit(1)
end
