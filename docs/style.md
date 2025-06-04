# Introduction
This coding style applies to all of the new code that's introduced into the codebase.
When fixing old code that might not adhere to this style guide, use common sense to decide whether this code style guide should be applied,
based on how much of the content is actually new, but never mix conventions on a function level. When not adhering to the style guide provide a rationale in a comment.

# General formatting
All code needs to be formatted according to rules defined in [pre-commit configuration](../.pre-commit-config.yaml) - specifally with regards to the clang-format and clang-tidy rules.
Apart from that there are style rules that can't be expressed in clang-format but must be followed nevertheless:

# Formatting quirks
## Boolean statements (e.g. in if) should explicitly use parenthesis and never rely on operators order:
  - Do: `((a != b) && (c != d))`
  - Don't: `(a != b && c != d)`
## In comparison statements the constant should be on the left of the comparison operator
  - Do: `if (0 < variable)`
  - Don't: `if (variable == 0)`
# Readability
- Always chose readability over compactness
- Never use side-effects in if statements if they are conditional:
    - Do:

            if (7 == a)
            {
              if (NULL == (y = malloc(10))
              {
              }
            }

    - Don't:

            if ((7 == a) && (NULL == (y = malloc(10))))
            {
            }


- Avoid ternary operators with complex statements
- Avoid deeply nested if statements: use if-else chains, helper functions, early returns or goto to cleanup code:
    - Do:

            void someFunction(int x)
            {
              int result = 0;
              char* foo = NULL;
              char* bar = NULL;
              char* baz = NULL;

              if (7 == x)
              {
                return EINVAL;
              }

              foo = func();
              if (NULL == foo)
              {
                result = ENOMEM;
                goto cleanup;
              }

              bar = func();
              if (NULL == bar)
              {
                result = ENOMEM;
                goto cleanup;
              }

              baz = func();
              if (NULL == baz)
              {
                result = ENOMEM;
                goto cleanup;
              }

              result = doSomething(foo, bar, baz);

            cleanup:
              if (NULL != foo)
              {
                free(foo);
              }
              if (NULL != bar)
              {
                free(bar);
              }
              if (NULL != baz)
              {
                free(baz);
              }
              return result;
            }

    - Don't:

            void someFunction(int x)
            {
              int result = 0;
              char* foo = NULL;
              char* bar = NULL;
              char* baz = NULL;
              foo = func();
              if (7 == x) {
                if (NULL != foo)
                {
                  bar = func();
                  if (NULL != bar)
                  {
                    baz = func();
                    if (NULL != bar)
                    {
                      result = doSomething(foo, bar, baz);
                    }
                    else
                    {
                      result = ENOMEM;
                    }
                  }
                  else
                  {
                    result = ENOMEM;
                  }
                }
                else
                {
                  result = ENOMEM;
                }
              }
              else
              {
                result = EINVAL;
              }

              if (NULL != foo)
              {
                free(foo);
              }
              if (NULL != bar)
              {
                free(bar);
              }
              if (NULL != baz)
              {
                free(baz);
              }
              return result;
            }


# Naming Conventions
- PascalCase for file names, function names, types (including class and struct), and namespaces
- camelCase for parameters and variables
- g_ prefix for global variables, then camelCase
- UPPER_CASE for macros

# Variables
-  Variables must always be initialized
-  For C, variables should be defined at the beginning of the code block they're used:
    - Do:

            int foo(int bar)
            {
              int y = 5;
              if (1 == bar)
              {
                int x=0;
                x = 7;
                return x;
              }
              return y;
            }

    - Don't:

            int foo(int bar)
            {
              int x=0; // variable defined at the beginning of a larger block, but it's used only in the smaller block
              if (1 == bar)
              {
                x = 7;
                return x;
              }
              int y = 5; // variable defined not at the beginning of the code block
              return y;
            }

- In C++, variables should be defined when needed
- Variables must not be reused for different purposes, except for simple index variables in for loops and a function-wide result variable
- Variable names must be verbose enough to explain their purpose if not obvious
- Use boolean true and false rather than 1 and 0 for boolean operations
- In C++ when passing an argument references should be used if object is mutable but not nullable, const references for non-mutable, non-nullable objects, pointers only for object that can be null.

# Error Handling
- For C, return integer status codes for functions that can fail, typically using standard errno values, and 0 for success
- For C++ using complex/optional error types is recommended
- Always validate function inputs
- Use early returns for validation and error conditions
- Log errors with descriptive messages using OsConfigLogError, OsConfigLogWarning, or OsConfigLogInfo
- For C++ avoid throwing exceptions, but handle them properly when thrown by external code, e.g.:

            try
            {
                valueRegex = regex(value);
            }
            catch (const std::exception& e)
            {
                OsConfigLogError(log, "Regex error: %s", e.what());
                return Error("Failed to compile regex '" + value + "' error: " + e.what());
            }

# Memory Management
- Always check for memory allocation failures, fail gracefully if possible
- Avoid dangling invalid pointers - if it's freed it should be NULL, use FREE_MEMORY(ptr) macro to safely free memory.
- Free all allocated memory (except output values) before function returns, including error paths
- Functions allocating memory should document ownership transfer to caller
- For C++ use smart pointers wherever possible, avoid new and delete, wrap manually allocated objects in e.g. std::unique_ptr<> with a custom deleter to avoid possible leaks

# Logging
- Include an OsConfigLogHandle log parameter in functions that need to log
- Use appropriate log levels:
  - OsConfigLogDebug: Detailed debugging information
  - OsConfigLogInfo: General operational information
  - OsConfigLogWarning: Non-critical warnings
  - OsConfigLogError: Function errors and failures
- Log return values and status codes for debugging
- Avoid logging sensitive data, eg. information read from the user machine on log levels lower than Debug

# Comments and Documentation
- Each source code file starts with a copyright notice
- Document non-obvious behavior or complex algorithms
- ...but don't be too verbose (if it can be easily found in man page it doesn't have to be documented)

# Testing
- All the new code must have reasonable unit test coverage
- Unit tests must:
  - not depend on each other nor interfere with each other (hint: use --gtest_shuffle)
  - be confined and not interfere with the system
  - not require special privileges if not needed
    - if privileges are required (e.g. testing changing file access) the test must detect that it's not being run privileged and be skipped
