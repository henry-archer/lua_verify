--[[
  Lua Schema Validator (validator.lua)
  Pure Lua table validation library (Lua 5.1 - 5.4+ compatible, zero dependencies)
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
        -- Dynamic threshold based on length of target key
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

-- Recursive validation worker
local function validate_internal(data, schema, path, errors, options)
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

        if val == nil then
            -- Field is missing
            if rule.required then
                table.insert(errors, string.format("Error at '%s': missing required field", current_path))
            end
            -- If optional and missing, do not validate further rules or nested schemas
        else
            -- Field is present
            local val_type = type(val)

            -- Type validation
            if rule.type then
                local type_matched = false
                if rule.type == "integer" then
                    -- Check if number and integer
                    if val_type == "number" then
                        if math.type then
                            type_matched = (math.type(val) == "integer")
                        else
                            type_matched = (val % 1 == 0)
                        end
                    end
                else
                    type_matched = (val_type == rule.type)
                end

                if not type_matched then
                    table.insert(errors, string.format("Error at '%s': expected type '%s', got '%s'", current_path, rule.type, val_type))
                end
            end

            -- Min limit validation
            if rule.min ~= nil then
                if val_type == "number" then
                    if val < rule.min then
                        table.insert(errors, string.format("Error at '%s': value %s is less than minimum allowed value %s", current_path, format_value(val), tostring(rule.min)))
                    end
                elseif rule.type == "number" or rule.type == "integer" then
                    -- Type error already reported or handled
                end
            end

            -- Max limit validation
            if rule.max ~= nil then
                if val_type == "number" then
                    if val > rule.max then
                        table.insert(errors, string.format("Error at '%s': value %s is greater than maximum allowed value %s", current_path, format_value(val), tostring(rule.max)))
                    end
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

            -- Nested schema validation
            if rule.schema ~= nil then
                if val_type == "table" then
                    validate_internal(val, rule.schema, current_path, errors, options)
                elseif rule.type == "table" or rule.type == nil then
                    -- If rule.type was not explicitly set to table, but schema was provided and val wasn't table
                    if not rule.type then
                        table.insert(errors, string.format("Error at '%s': expected table for nested schema, got '%s'", current_path, val_type))
                    end
                end
            end
        end
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
-- @param options (table, optional) Additional validation options:
--        - allow_unknown_keys (boolean, default false): Allow keys not listed in schema.
--        - suggest_typos (boolean, default true): Provide typo hints for unknown keys.
-- @return ok (boolean) true if configuration is valid, false otherwise.
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

return validator
