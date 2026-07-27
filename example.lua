--[[
  HPC Simulation Configuration Validator Example (example.lua)
  Demonstrates validating a complex, nested Lua simulation configuration
  against a schema before feeding data to a Fortran backend.
--]]

local validator = require("validator")

-- 1. Define the Schema for the HPC Simulation
local hpc_schema = {
    simulation_name = {
        type = "string",
        required = true
    },
    time_step = {
        type = "number",
        required = true,
        min = 1e-6,
        max = 1.0
    },
    max_steps = {
        type = "integer",
        required = true,
        min = 1,
        max = 1000000
    },
    -- Optional parent table for Physics parameters
    physics = {
        type = "table",
        required = false, -- Optional parent
        schema = {
            dimension = {
                type = "integer",
                required = true,
                min = 1,
                max = 3
            },
            fluid = {
                type = "table",
                required = true, -- Required child if 'physics' is present
                schema = {
                    density = { type = "number", required = true, min = 0.0 },
                    viscosity = { type = "number", required = true, min = 0.0 }
                }
            }
        }
    },
    -- Optional parent table for Linear Solver settings
    solver = {
        type = "table",
        required = false,
        schema = {
            method = {
                type = "string",
                required = true,
                enum = {"cg", "gmres", "bicgstab"}
            },
            tolerance = {
                type = "number",
                required = true,
                min = 1e-15,
                max = 1e-2
            },
            preconditioner = {
                type = "string",
                required = false,
                enum = {"none", "jacobi", "ilu", "amg"}
            }
        }
    }
}

-- Print separator helper
local function print_header(title)
    print("\n==================================================")
    print(" " .. title)
    print("==================================================")
end

-- Function to run validation and print formatted output
local function run_demo(label, config)
    print_header(label)
    local ok, errors = validator.validate(config, hpc_schema)

    if ok then
        print("Status: VALID [Passed Validation]")
        print("Config ready to populate Fortran data containers safely!")
    else
        print("Status: INVALID [Validation Failed]")
        print(string.format("Found %d error(s):", #errors))
        print("--------------------------------------------------")
        print(tostring(errors))
    end
end

-- Scenario 1: Valid Configuration with all fields populated
local valid_config = {
    simulation_name = "Couette Flow 3D",
    time_step = 0.001,
    max_steps = 10000,
    physics = {
        dimension = 3,
        fluid = {
            density = 1000.0,
            viscosity = 0.001
        }
    },
    solver = {
        method = "gmres",
        tolerance = 1e-8,
        preconditioner = "ilu"
    }
}

-- Scenario 2: Valid Minimal Config (Optional parent 'solver' omitted)
local minimal_valid_config = {
    simulation_name = "Minimal Test",
    time_step = 0.05,
    max_steps = 500
}

-- Scenario 3: Config with multiple bugs (Typo, Type mismatch, Min/Max out of bounds, Missing required sub-field)
local invalid_config = {
    simulation_name = "Broken Setup",
    time_stpe = 0.01, -- Typo in key! ('time_stpe' instead of 'time_step')
    max_steps = "10000", -- String instead of integer (strict typing!)
    physics = {
        dimension = 4, -- Exceeds max 3
        fluid = {
            density = -5.0 -- Less than min 0.0 (viscosity is also missing!)
        }
    },
    solver = {
        method = "super_solver", -- Invalid enum value
        tolerance = 1e-20 -- Less than min 1e-15
    }
}

-- Run Demos
run_demo("Scenario 1: Valid Full Configuration", valid_config)
run_demo("Scenario 2: Valid Minimal Configuration (Optional Parents Omitted)", minimal_valid_config)
run_demo("Scenario 3: Invalid Configuration (Errors Accumulated)", invalid_config)
