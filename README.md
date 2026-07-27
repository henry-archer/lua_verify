# Lua Schema Validator (`validator.lua`)

A lightweight, high-performance, pure Lua schema validator and documentation generator (Lua 5.1 – 5.4+ compatible with **zero dependencies**).

Designed to validate nested user configuration tables before populating HPC simulation data structures (e.g., Fortran backends) to prevent bad memory allocations, invalid parameters, or runtime crashes.

---

## Features

- **Pure Lua & Zero Dependencies**: Fully portable, works out of the box with Lua 5.1, 5.2, 5.3, 5.4, 5.5, and LuaJIT.
- **Strict Typing & Union Types**: No implicit type coercion (e.g., `"10"` will not pass for a `number` field). Supports union types (e.g., `type = {"number", "string"}`).
- **Array / List Validation (`array_of`)**: Validate elements of array/sequence tables, including arrays of primitive types or arrays of sub-tables (`boundary_conditions[1].type`).
- **Custom Validation Functions (`custom`)**: Define arbitrary domain-specific validation logic (e.g., physics checks or CFL stability criteria).
- **Auto-Generate Schema Documentation (`to_markdown`)**: Generate clean, professional Markdown documentation tables directly from your schema definitions.
- **Nested Table Schemas**: Supports arbitrarily deep sub-table validation with precise dot-separated error paths (`physics.fluid.viscosity`).
- **Optional Parents & Required Children**: Omitted optional parent tables pass validation cleanly, but if provided, all required sub-fields inside are strictly enforced.
- **Unknown Key Detection & Typo Hints**: Detects misspelled configuration keys (e.g., `time_stpe` instead of `time_step`) using Levenshtein distance and suggests closest key corrections.
- **Complete Error Accumulation**: Does not fail fast on the first error; gathers **all** validation errors across the entire configuration so users can debug their config files in a single pass.

---

## Schema Rules API

Each key in the schema table corresponds to an expected key in the user config. The value is a table specifying validation rules:

| Rule Field | Type | Description |
|---|---|---|
| `type` | String or Array | Expected Lua type (`"number"`, `"integer"`, `"string"`, `"boolean"`, `"table"`, or array of types). |
| `required` | Boolean | If `true`, key must be present in config. If `false` or omitted, key is optional. |
| `nullable` | Boolean | If `true`, `nil` values are accepted even if required/typed. |
| `min` / `max` | Number | Inclusive minimum / maximum boundary for numeric types. |
| `min_len` / `max_len` | Integer | Minimum / maximum length for strings or arrays/tables. |
| `enum` | Table/Array | Array of allowed literal values (e.g. `{"cg", "gmres", "bicgstab"}`). |
| `array_of` | Table | Rule specification for sequence/array elements. |
| `schema` | Table | Nested dictionary schema definition table if `type` is `"table"`. |
| `custom` | Function | Custom function `function(val, path)` returning `ok, err_msg`. |
| `description` | String | (Metadata) Parameter description for documentation generation. |
| `default` | Any | (Metadata) Default parameter value for documentation generation. |
| `units` | String | (Metadata) Parameter physical units (e.g., `"kg/m^3"`, `"s"`). |

---

## Usage Example

```lua
local validator = require("validator")

-- 1. Define Schema with Metadata & Array Validation
local schema = {
    simulation_name = {
        type = "string",
        required = true,
        description = "Simulation title"
    },
    time_step = {
        type = "number",
        required = true,
        min = 1e-6,
        max = 1.0,
        default = 0.01,
        units = "s",
        description = "Integration time step"
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

-- 2. Validate User Config
local user_config = {
    simulation_name = "Thermal Flow",
    time_step = 0.001,
    boundary_conditions = {
        { name = "inlet", value = 100.0 }
    }
}

local ok, errors = validator.validate(user_config, schema)
if not ok then
    print(tostring(errors))
end

-- 3. Auto-Generate Markdown Documentation
local docs_md = validator.to_markdown(schema, "Simulation Configuration Guide")
print(docs_md)
```

---

## Running the Examples and Test Suite

### 1. Run Demonstration Script
```bash
lua example.lua
```

### 2. Run Test Suite
```bash
lua test_validator.lua
```
