--[[
  Enhanced HPC Simulation Configuration Validator Example (example.lua)
  Demonstrates validating complex Lua configs, array validation, custom rules,
  and auto-generating Markdown schema documentation for users.
--]]

local validator = require("validator")

-- 1. Define the Schema for the HPC Simulation with Metadata & Array/Custom Rules
local hpc_schema = {
    simulation_name = {
        type = "string",
        required = true,
        description = "Unique title identifier for the simulation run"
    },
    time_step = {
        type = "number",
        required = true,
        min = 1e-6,
        max = 1.0,
        default = 0.01,
        units = "s",
        description = "Integration time step size"
    },
    max_steps = {
        type = "integer",
        required = true,
        min = 1,
        max = 1000000,
        default = 10000,
        units = "dimensionless",
        description = "Maximum number of time steps to compute"
    },
    -- List/Array of boundary condition tables
    boundary_conditions = {
        type = "table",
        required = true,
        description = "List of boundary conditions for the physical domain",
        array_of = {
            type = "table",
            schema = {
                name = {
                    type = "string",
                    required = true,
                    description = "Boundary patch identifier"
                },
                type = {
                    type = "string",
                    required = true,
                    enum = {"dirichlet", "neumann", "periodic"},
                    description = "Boundary condition mathematical type"
                },
                value = {
                    type = "number",
                    required = true,
                    description = "Prescribed value at boundary"
                }
            }
        }
    },
    -- Optional parent table for Physics parameters
    physics = {
        type = "table",
        required = false,
        description = "Physical environment and fluid parameters",
        schema = {
            dimension = {
                type = "integer",
                required = true,
                min = 1,
                max = 3,
                default = 3,
                description = "Spatial dimension of the simulation (1, 2, or 3)"
            },
            fluid = {
                type = "table",
                required = true,
                description = "Fluid physical properties",
                schema = {
                    density = {
                        type = "number",
                        required = true,
                        min = 0.0,
                        units = "kg/m^3",
                        description = "Fluid mass density"
                    },
                    viscosity = {
                        type = "number",
                        required = true,
                        min = 0.0,
                        units = "Pa.s",
                        description = "Dynamic viscosity of the fluid"
                    }
                }
            }
        }
    },
    -- Optional parent table for Linear Solver settings with a Custom Rule
    solver = {
        type = "table",
        required = false,
        description = "Iterative linear system solver settings",
        schema = {
            method = {
                type = "string",
                required = true,
                enum = {"cg", "gmres", "bicgstab"},
                default = "gmres",
                description = "Iterative Krylov solver method"
            },
            tolerance = {
                type = "number",
                required = true,
                min = 1e-15,
                max = 1e-2,
                default = 1e-8,
                units = "dimensionless",
                description = "Convergence residual tolerance",
                -- Custom validation function rule
                custom = function(val, path)
                    if val > 1e-3 then
                        return false, string.format("Warning at '%s': tolerance %g is unusually loose for high-precision HPC", path, val)
                    end
                    return true
                end
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

-- Scenario 1: Valid Configuration with all fields and arrays
local valid_config = {
    simulation_name = "Couette Flow 3D",
    time_step = 0.001,
    max_steps = 10000,
    boundary_conditions = {
        { name = "inlet", type = "dirichlet", value = 1.0 },
        { name = "outlet", type = "neumann", value = 0.0 }
    },
    physics = {
        dimension = 3,
        fluid = {
            density = 1000.0,
            viscosity = 0.001
        }
    },
    solver = {
        method = "gmres",
        tolerance = 1e-8
    }
}

-- Scenario 2: Invalid Configuration (Array errors, Typo, Custom Rule Triggered)
local invalid_config = {
    simulation_name = "Broken Setup",
    time_stpe = 0.01, -- Typo in key! ('time_stpe')
    max_steps = 10000,
    boundary_conditions = {
        { name = "inlet", type = "invalid_bc_type", value = 1.0 }, -- Bad enum in array [1]
        { name = "outlet", type = "neumann" } -- Missing 'value' in array [2]
    },
    solver = {
        method = "gmres",
        tolerance = 0.01 -- Fails custom precision check (> 1e-3)
    }
}

-- Run Demos
run_demo("Scenario 1: Valid Full Configuration (With Arrays)", valid_config)
run_demo("Scenario 2: Invalid Configuration (Array & Custom Errors)", invalid_config)

-- Scenario 3: Auto-Generate Schema Documentation
print_header("Scenario 3: Auto-Generated Markdown Schema Documentation")
local markdown_docs = validator.to_markdown(hpc_schema, "HPC Simulation Parameter Reference")
print(markdown_docs)
