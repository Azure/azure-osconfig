#ifndef MODULESMANAGERTESTS_H
#define MODULESMANAGERTESTS_H

// Path to the test modules directory
#define TEST_MODULE_DIR "/home/ubuntu/git/azure-osconfig/build/platform/tests/bin"

// Path to each test module
#define TEST_VALID_MODULE_PATH_V1 "/home/ubuntu/git/azure-osconfig/build/platform/tests/bin/valid_module_v1.so"
#define TEST_VALID_MODULE_PATH_V2 "/home/ubuntu/git/azure-osconfig/build/platform/tests/bin/valid_module_v2.so"
#define TEST_INVALID_MODULE_PATH "/home/ubuntu/git/azure-osconfig/build/platform/tests/bin/invalid_module.so"
#define TEST_INVALID_GETINFO_MODULE_PATH "/home/ubuntu/git/azure-osconfig/build/platform/tests/bin/invalidgetinfo_module.so"

// Component names used by all valid modules
#define TEST_MODULE_COMPONENT_1 "TestModule_Component_1"
#define TEST_MODULE_COMPONENT_2 "TestModule_Component_2"

#define TEST_CONFIG_JSON_INVALID "/home/ubuntu/git/azure-osconfig/build/platform/tests/osconfig/osconfig-invalid.json"
#define TEST_CONFIG_JSON_NONE_REPORTED "/home/ubuntu/git/azure-osconfig/build/platform/tests/osconfig/osconfig-none-reported.json"
#define TEST_CONFIG_JSON_SINGLE_REPORTED "/home/ubuntu/git/azure-osconfig/build/platform/tests/osconfig/osconfig-single-reported.json"
#define TEST_CONFIG_JSON_MULTIPLE_REPORTED "/home/ubuntu/git/azure-osconfig/build/platform/tests/osconfig/osconfig-multiple-reported.json"

// Object names
#define TEST_OBJECT_STRING "string"
#define TEST_OBJECT_INTEGER "integer"
#define TEST_OBJECT_BOOLEAN "boolean"
#define TEST_OBJECT_INTEGER_ENUM "integerEnum"
#define TEST_OBJECT_INTEGER_ARRAY "integerArray"
#define TEST_OBJECT_STRING_ARRAY "stringArray"
#define TEST_OBJECT_INTEGER_MAP "integerMap"
#define TEST_OBJECT_STRING_MAP "stringMap"
#define TEST_OBJECT "object"
#define TEST_OBJECT_ARRAY "objectArray"

// Object payloads
#define TEST_OBJECT_STRING_PAYLOAD "\"string\""
#define TEST_OBJECT_INTEGER_PAYLOAD "123"
#define TEST_OBJECT_BOOLEAN_PAYLOAD "true"
#define TEST_OBJECT_INTEGER_ENUM_PAYLOAD "1"
#define TEST_OBJECT_INTEGER_ARRAY_PAYLOAD "[1, 2, 3]"
#define TEST_OBJECT_STRING_ARRAY_PAYLOAD "[\"a\", \"b\", \"c\"]"
#define TEST_OBJECT_INTEGER_MAP_PAYLOAD "{\"key1\": 1, \"key2\": 2}"
#define TEST_OBJECT_STRING_MAP_PAYLOAD "{\"key1\": \"a\", \"key2\": \"b\"}"
#define TEST_OBJECT_PAYLOAD "{" \
        "\"string\": \"value\"," \
        "\"integer\": 1," \
        "\"boolean\": true," \
        "\"integerEnum\": 1," \
        "\"integerArray\": [1, 2, 3]," \
        "\"stringArray\": [\"a\", \"b\", \"c\"]," \
        "\"integerMap\": { \"key1\": 1, \"key2\": 2 }," \
        "\"stringMap\": { \"key1\": \"a\", \"key2\": \"b\" }" \
    "}"
#define TEST_OBJECT_ARRAY_PAYLOAD "[" \
        "{" \
            "\"string\": \"value\"," \
            "\"integer\": 1," \
            "\"boolean\": true," \
            "\"integerEnum\": 1," \
            "\"integerArray\": [1, 2, 3]," \
            "\"stringArray\": [\"a\", \"b\", \"c\"]," \
            "\"integerMap\": { \"key1\": 1, \"key2\": 2 }," \
            "\"stringMap\": { \"key1\": \"a\", \"key2\": \"b\" }" \
        "}," \
        "{" \
            "\"string\": \"value\"," \
            "\"integer\": 1," \
            "\"boolean\": true," \
            "\"integerEnum\": 1," \
            "\"integerArray\": [1, 2, 3]," \
            "\"stringArray\": [\"a\", \"b\", \"c\"]," \
            "\"integerMap\": { \"key1\": 1, \"key2\": 2 }," \
            "\"stringMap\": { \"key1\": \"a\", \"key2\": \"b\" }" \
        "}" \
    "]"

// Payloads for testing MpiSetDesired() and MpiGetReported()
#define TEST_SINGLE_OBJECT_PAYLOAD "{" \
    "\"TestModule_Component_1\": {" \
        "\"string\": \"string\"" \
    "}" \
"}"
#define TEST_MULTIPLE_OBJECT_PAYLOAD "{" \
    "\"TestModule_Component_1\": {" \
        "\"string\": \"string\"," \
        "\"integer\": 123," \
        "\"boolean\": true," \
        "\"integerEnum\": 1," \
        "\"integerArray\": [1, 2, 3]," \
        "\"stringArray\": [\"a\", \"b\", \"c\"]," \
        "\"integerMap\": { \"key1\": 1, \"key2\": 2 }," \
        "\"stringMap\": { \"key1\": \"a\", \"key2\": \"b\" }," \
        "\"object\": {" \
            "\"string\": \"value\"," \
            "\"integer\": 1," \
            "\"boolean\": true," \
            "\"integerEnum\": 1," \
            "\"integerArray\": [1, 2, 3]," \
            "\"stringArray\": [\"a\", \"b\", \"c\"]," \
            "\"integerMap\": { \"key1\": 1, \"key2\": 2 }," \
            "\"stringMap\": { \"key1\": \"a\", \"key2\": \"b\" }" \
        "}," \
        "\"objectArray\": [" \
            "{" \
                "\"string\": \"value\"," \
                "\"integer\": 1," \
                "\"boolean\": true," \
                "\"integerEnum\": 1," \
                "\"integerArray\": [1, 2, 3]," \
                "\"stringArray\": [\"a\", \"b\", \"c\"]," \
                "\"integerMap\": { \"key1\": 1, \"key2\": 2 }," \
                "\"stringMap\": { \"key1\": \"a\", \"key2\": \"b\" }" \
            "}," \
            "{" \
                "\"string\": \"value\"," \
                "\"integer\": 1," \
                "\"boolean\": true," \
                "\"integerEnum\": 1," \
                "\"integerArray\": [1, 2, 3]," \
                "\"stringArray\": [\"a\", \"b\", \"c\"]," \
                "\"integerMap\": { \"key1\": 1, \"key2\": 2 }," \
                "\"stringMap\": { \"key1\": \"a\", \"key2\": \"b\" }" \
            "}" \
        "]" \
    "}" \
"}"

#endif // MODULESMANAGERTESTS_H
