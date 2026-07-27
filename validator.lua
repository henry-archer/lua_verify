--[[
  Lua Schema Validator (validator.lua)
  Pure Lua table validation & documentation library (Lua 5.1 - 5.4+ compatible, zero dependencies)
--]]

local validator = {}

-- Utility: Levenshtein distance between two strings for typo suggestions
local function levenshtein(s1, s2)
    local len1, len2 = #s1, #s2
    local matrix = {}
    for i = 0, len1 do
        matrix[i] = {[0] = i}
    end
    for j = 0, len2 do
        matrix[0][j] = j
    end
    for i = 1, len1 do
        for j = 1, len2 do
            local cost = (s1:sub(i, i) == s2:sub(j, j)) and 0 or 1
            matrix[i][j] = math.min(
                matrix[i - 1][j] + 1,
                matrix[i][j - 1] + 1,
                matrix[i - 1][j - 1] + cost
            )
        end
    end
    return matrix[len1][len2]
end

-- Find closest matching key in schema for typo hints
local function find_closest_key(target, schema_keys)
    local best_match = nil
    local min_dist = math.huge

    for _, key in ipairs(schema_keys) do
        local key_str = tostring(key)
        local dist = levenshtein(target, key_str)
        local max_allowed = math.max(2, math.floor(#target / 2))
        if dist < min_dist and dist <= max_allowed then
            min_dist = dist
            best_match = key_str
        end
    end

    return best_match
end

-- Helper to format path for reporting
local function make_path(prefix, key)
    local key_str = tostring(key)
    if prefix == nil or prefix == "" then
        return key_str
    else
        return prefix .. "." .. key_str
    end
end

-- Helper to format values for error messages
local function format_value(val)
    if type(val) == "string" then
        return string.format("%q", val)
    else
        return tostring(val)
    end
end

-- Helper to check single type
local function check_single_type(val, expected_type)
    local val_type = type(val)
    if expected_type == "integer" then
        if val_type == "number" then
            if math.type then
                return math.type(val) == "integer"
            else
                return val % 1 == 0
            end
        end
        return false
    else
        return val_type == expected_type
    end
end

-- Helper to check single or multiple types
local function check_type(val, type_rule)
    if type(type_rule) == "string" then
        return check_single_type(val, type_rule)
    elseif type(type_rule) == "table" then
        for _, t in ipairs(type_rule) do
            if check_single_type(val, t) then
                return true
            end
        end
        return false
    end
    return true
end

-- Format type string for error message
local function format_type_rule(type_rule)
    if type(type_rule) == "table" then
        return table.concat(type_rule, " or ")
    else
        return tostring(type_rule)
    end
end

-- Helper to check if a table is array-like (sequence)
local function is_array(t)
    if type(t) ~= "table" then return false end
    local count = 0
    for k, _ in pairs(t) do
        if type(k) ~= "number" or k < 1 or k % 1 ~= 0 then
            return false
        end
        count = count + 1
    end
    for i = 1, count do
        if t[i] == nil then return false end
    end
    return true
end

-- Forward declaration of internal validation
local validate_internal

-- Validate a value against a rule definition
local function validate_rule(val, rule, current_path, errors, options)
    local val_type = type(val)

    -- Nullable check
    if val == nil then
        if rule.nullable then
            return
        end
        if rule.required then
            table.insert(errors, string.format("Error at '%s': missing required field", current_path))
        end
        return
    end

    -- Type validation
    if rule.type then
        if not check_type(val, rule.type) then
            table.insert(errors, string.format("Error at '%s': expected type '%s', got '%s'", current_path, format_type_rule(rule.type), val_type))
            return
        end
    end

    -- Min limit validation (for numbers)
    if rule.min ~= nil and val_type == "number" then
        if val < rule.min then
            table.insert(errors, string.format("Error at '%s': value %s is less than minimum allowed value %s", current_path, format_value(val), tostring(rule.min)))
        end
    end

    -- Max limit validation (for numbers)
    if rule.max ~= nil and val_type == "number" then
        if val > rule.max then
            table.insert(errors, string.format("Error at '%s': value %s is greater than maximum allowed value %s", current_path, format_value(val), tostring(rule.max)))
        end
    end

    -- Min/Max length validation (for strings and tables/arrays)
    if rule.min_len ~= nil then
        local len = (val_type == "string" and #val) or (val_type == "table" and #val)
        if len and len < rule.min_len then
            table.insert(errors, string.format("Error at '%s': length %d is less than minimum length %d", current_path, len, rule.min_len))
        end
    end

    if rule.max_len ~= nil then
        local len = (val_type == "string" and #val) or (val_type == "table" and #val)
        if len and len > rule.max_len then
            table.insert(errors, string.format("Error at '%s': length %d is greater than maximum length %d", current_path, len, rule.max_len))
        end
    end

    -- Enum validation
    if rule.enum ~= nil then
        local in_enum = false
        local allowed_str_list = {}
        for _, allowed_val in ipairs(rule.enum) do
            table.insert(allowed_str_list, format_value(allowed_val))
            if val == allowed_val then
                in_enum = true
            end
        end

        if not in_enum then
            table.insert(errors, string.format("Error at '%s': value %s is not in allowed list [%s]", current_path, format_value(val), table.concat(allowed_str_list, ", ")))
        end
    end

    -- Custom function validation
    if rule.custom ~= nil and type(rule.custom) == "function" then
        local ok_c, err_c = rule.custom(val, current_path)
        if not ok_c then
            table.insert(errors, err_c or string.format("Error at '%s': custom validation failed", current_path))
        end
    end

    -- Array elements validation (`array_of`)
    if rule.array_of ~= nil and val_type == "table" then
        if not is_array(val) and #val == 0 and next(val) ~= nil then
            table.insert(errors, string.format("Error at '%s': expected array sequence, got key-value dictionary table", current_path))
        else
            for i, elem in ipairs(val) do
                local elem_path = string.format("%s[%d]", current_path, i)
                validate_rule(elem, rule.array_of, elem_path, errors, options)
            end
        end
    end

    -- Nested dictionary schema validation (`schema`)
    if rule.schema ~= nil and val_type == "table" then
        validate_internal(val, rule.schema, current_path, errors, options)
    end
end

-- Recursive validation worker for schema tables
validate_internal = function(data, schema, path, errors, options)
    if type(data) ~= "table" then
        table.insert(errors, string.format("Error at '%s': expected table, got %s", path ~= "" and path or "root", type(data)))
        return
    end

    if type(schema) ~= "table" then
        table.insert(errors, string.format("Error at '%s': invalid schema definition (expected table)", path ~= "" and path or "root"))
        return
    end

    -- Collect defined schema keys
    local schema_keys = {}
    for k, _ in pairs(schema) do
        table.insert(schema_keys, k)
    end

    -- 1. Check for Unknown Keys in user data
    if not options.allow_unknown_keys then
        for k, _ in pairs(data) do
            if schema[k] == nil then
                local current_path = make_path(path, k)
                local hint = ""
                if options.suggest_typos ~= false then
                    local suggestion = find_closest_key(tostring(k), schema_keys)
                    if suggestion then
                        hint = string.format(". Did you mean '%s'?", suggestion)
                    end
                end
                table.insert(errors, string.format("Error at '%s': unknown key%s", current_path, hint))
            end
        end
    end

    -- 2. Validate expected schema rules
    for k, rule in pairs(schema) do
        local current_path = make_path(path, k)
        local val = data[k]
        validate_rule(val, rule, current_path, errors, options)
    end
end

-- Custom error table with formatted printing
local function create_error_result(errors)
    local err_obj = {}
    for i, e in ipairs(errors) do
        err_obj[i] = e
    end
    setmetatable(err_obj, {
        __tostring = function(t)
            return table.concat(t, "\n")
        end
    })
    return err_obj
end

--- Validate a configuration table against a schema definition.
-- @param config (table) The user configuration table to validate.
-- @param schema (table) The schema rules definition.
-- @param options (table, optional) Additional validation options.
-- @return ok (boolean) true if valid, false otherwise.
-- @return errors (table/string) List of error strings (supports tostring()).
function validator.validate(config, schema, options)
    options = options or {}
    if options.allow_unknown_keys == nil then
        options.allow_unknown_keys = false
    end
    if options.suggest_typos == nil then
        options.suggest_typos = true
    end

    local errors = {}
    validate_internal(config, schema, "", errors, options)

    if #errors == 0 then
        return true, nil
    else
        return false, create_error_result(errors)
    end
end

--------------------------------------------------------------------------------
-- SCHEMA DOCUMENTATION GENERATOR (`to_markdown`)
--------------------------------------------------------------------------------

local function build_doc_rows(schema, prefix, rows)
    for k, rule in pairs(schema) do
        local param_name = (prefix == "" and k or prefix .. "." .. k)
        local type_str = format_type_rule(rule.type or "any")
        local req_str = rule.required and "**Yes**" or "No"
        
        -- Bounds or Enum
        local constraint_str = "-"
        if rule.enum then
            local formatted_enums = {}
            for _, v in ipairs(rule.enum) do
                table.insert(formatted_enums, format_value(v))
            end
            constraint_str = "`" .. table.concat(formatted_enums, " | ") .. "`"
        elseif rule.min ~= nil or rule.max ~= nil then
            local min_s = rule.min ~= nil and tostring(rule.min) or "-∞"
            local max_s = rule.max ~= nil and tostring(rule.max) or "∞"
            constraint_str = string.format("`[%s, %s]`", min_s, max_s)
        end

        local default_str = rule.default ~= nil and ("`" .. tostring(rule.default) .. "`") or "-"
        local units_str = rule.units ~= nil and ("`" .. tostring(rule.units) .. "`") or "-"
        local desc_str = rule.description or "-"

        table.insert(rows, {
            param = "`" .. param_name .. "`",
            type = "`" .. type_str .. "`",
            required = req_str,
            constraint = constraint_str,
            default = default_str,
            units = units_str,
            description = desc_str
        })

        -- Recurse into nested schemas
        if rule.schema then
            build_doc_rows(rule.schema, param_name, rows)
        end

        -- Recurse into array element schemas
        if rule.array_of then
            local arr_prefix = param_name .. "[]"
            if rule.array_of.schema then
                build_doc_rows(rule.array_of.schema, arr_prefix, rows)
            else
                local arr_type = format_type_rule(rule.array_of.type or "any")
                local arr_desc = rule.array_of.description or ("Array element of " .. param_name)
                table.insert(rows, {
                    param = "`" .. arr_prefix .. "`",
                    type = "`" .. arr_type .. "`",
                    required = "N/A",
                    constraint = "-",
                    default = "-",
                    units = rule.array_of.units and ("`" .. tostring(rule.array_of.units) .. "`") or "-",
                    description = arr_desc
                })
            end
        end
    end
end

--- Auto-generate Markdown documentation from a schema definition table.
-- @param schema (table) Schema definition table.
-- @param title (string, optional) Document title.
-- @return markdown (string) Generated Markdown documentation string.
function validator.to_markdown(schema, title)
    title = title or "Configuration Schema Reference"
    local lines = {}
    table.insert(lines, "# " .. title)
    table.insert(lines, "")
    table.insert(lines, "| Parameter | Type | Required | Range / Enum | Default | Units | Description |")
    table.insert(lines, "|---|---|:---:|---|---|---|---|")

    local rows = {}
    build_doc_rows(schema, "", rows)

    -- Sort rows alphabetically by parameter path
    table.sort(rows, function(a, b) return a.param < b.param end)

    for _, row in ipairs(rows) do
        table.insert(lines, string.format("| %s | %s | %s | %s | %s | %s | %s |",
            row.param, row.type, row.required, row.constraint, row.default, row.units, row.description))
    end

    table.insert(lines, "")
    return table.concat(lines, "\n")
end

return validator
