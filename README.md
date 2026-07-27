# Lua Schema Validator (`validator.lua`)

A lightweight, high-performance, pure Lua schema validator (Lua 5.1 – 5.4+ compatible with **zero dependencies**).

Designed to validate nested user configuration tables before populating HPC simulation data structures (e.g., Fortran backends) to prevent bad memory allocations, invalid parameters, or runtime crashes.

---

## Key Features

- **Pure Lua & Zero Dependencies**: Fully portable, works out of the box with Lua 5.1, 5.2, 5.3, 5.4, 5.5, and LuaJIT.
- **Strict Typing**: No implicit type coercion (e.g., `"10"` will not pass for a `number` field).
- **Nested Table Schemas**: Supports arbitrarily deep sub-table validation with precise dot-separated error paths (`physics.fluid.viscosity`).
- **Optional Parents & Required Children**: Omitted optional parent tables pass validation cleanly, but if provided, all required sub-fields inside are strictly enforced.
- **Unknown Key Detection & Typo Hints**: Detects misspelled configuration keys (e.g., `time_stpe` instead of `time_step`) using Levenshtein distance and suggests closest key corrections.
- **Complete Error Accumulation**: Does not fail fast on the first error; gathers **all** validation errors across the entire configuration so users can debug their config files in a single pass.

---

## Schema Rules API

Each key in the schema table corresponds to an expected key in the user config. The value is a table specifying validation rules:

| Rule Field | Type | Description |
|---|---|---|
| `type` | String | Expected Lua type (`"number"`, `"integer"`, `"string"`, `"boolean"`, `"table"`, etc.). |
| `required` | Boolean | If `true`, key must be present in config. If `false` or omitted, key is optional. |
| `min` | Number | Inclusive minimum boundary for numeric types. |
| `max` | Number | Inclusive maximum boundary for numeric types. |
| `enum` | Table/Array | Array of allowed literal values (e.g. `{"cg", "gmres", "bicgstab"}`). |
| `schema` | Table | Nested schema definition table if `type` is `"table"`. |

---

## Quick Usage Example

```lua
local validator = require("validator")

-- Define Schema
local schema = {
    simulation_name = { type = "string", required = true },
    time_step = { type = "number", required = true, min = 1e-6, max = 1.0 },
    solver = {
        type = "table",
        required = false, -- Optional parent
        schema = {
            method = { type = "string", required = true, enum = {"cg", "gmres", "bicgstab"} },
            tolerance = { type = "number", required = true, min = 1e-15, max = 1e-2 }
        }
    }
}

-- User Config Table
local user_config = {
    simulation_name = "Thermal Flow",
    time_step = 0.001,
    solver = {
        method = "gmres",
        tolerance = 1e-8
    }
}

-- Validate
local ok, errors = validator.validate(user_config, schema)

if ok then
    print("Configuration is valid!")
else
    print("Validation failed:")
    print(tostring(errors))
end
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
