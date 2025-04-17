# Introduction

Azure Device OS Configuration (OSConfig) is a modular configuration stack for Linux Edge devices. OSConfig supports multi-authority device management over Azure and Azure Portal/CLI (via Azure PnP, IoT Hub, Azure Policy), GitOps, as well as local management (such as from Out Of Box Experience, OOBE). For more information on OSConfig see [OSConfig North Star Architecture](docs/architecture.md).

# Code of conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

# Contributing

We welcome contributions to OSConfig. The main way of contributing to and extending OSConfig is via developing new **Management Modules**. For more information see [OSConfig Management Modules](docs/modules.md).
Pull requests with few exceptions must contain appropriate unit-tests. We cannot allow test coverage to go down. Pull requests containing code changes without accompanying unit tests may be rejected.


Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com. When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

# Coding style
All code needs to be formatted according to .pre-commit-config.yaml. Each pull request is checked by [Formatting Tests](https://github.com/Azure/azure-osconfig/blob/dev/.github/workflows/formatting.yml).
Apart from that there are style rules that can't be expressed in clang-format but must be followed nevertheless:

## Formatting quirks
### Boolean statements (e.g. in if) should explicitly use parenthesis and never rely on operators order:
  - Bad: `(a != b && c != d)`
  - Good: `((a != b) && (c != d))`
### In comparison statements the constant should be on the left of the comparison operator
  - Bad: `if (variable == 0)`
  - Good: `if (0 < variable)`
## Readability
- Always chose readability over compactness
- Avoid side-effects in if statements, never use side-effects in if statements if they are conditional:
    - Bad:

            if (NULL == (x = malloc(10))) // Avoid
            {
            }

            if ((7 == a) && (NULL == (y = malloc(10)))) // Must not use
            {
            }


    - Good:

            x = malloc(10);
            if (NULL == x)
            {
            }

            if (7 == a)
            {
              y = malloc(10);
              if (NULL == y)
              {
              }
            }

- Avoid ternary operators with complex statements
- Avoid deeply nested if statements, use helper functions, early returns or goto to cleanup code:
    - Bad:

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

    - Good:

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


## Naming Conventions
- PascalCase for functions, types, and structs
- camelCase for parameters and variables
- g_ prefix for global variables
- UPPER_CASE for macros
## Variables
-  Variables must always be initialized
-  For C, variables should be defined at the beginning of the code block they're used:
    - Bad:

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

    - Good:

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

- Variables must not be reused for different purposes, except for simple index variables in for loops and a function-wide result variable
- Variable names must be verbose enough to explain their purpose if not obvious
- Use boolean true and false rather than 1 and 0 for boolean operations
## Error Handling
- For C, return integer status codes for functions that can fail, typically using standard errno values, and 0 for success
- For C++ using complex/optional error types is allowed
- Always validate function inputs
- Use early returns for validation and error conditions
- Log errors with descriptive messages using OsConfigLogError or OsConfigLogInfo
- For C++ avoid throwing exceptions, but handle them properly when thrown by external code
## Memory Management
- Always check for memory allocation failures, fail gracefully if possible
- Avoid dangling invalid pointers - if it's freed it should be NULL, use FREE_MEMORY(ptr) macro to safely free memory.
- Free all allocated memory before function returns, including error paths
- Functions allocating memory should document ownership transfer to caller
- For C++ use smart pointers wherever possible, avoid new and delete
## Logging
- Include an OsConfigLogHandle log parameter in functions that need to log
- Use appropriate log levels:
  - OsConfigLogDebug: Detailed debugging information
  - OsConfigLogInfo: General operational information
  - OsConfigLogError: Function errors and failures
- Log return values and status codes for debugging
- Avoid logging sensitive data, eg. information read from the user machine
## Comments and Documentation
- Each source code file starts with a copyright notice
- Document non-obvious behavior or complex algorithms
- ...but don't be too verbose (if it can be easily found in man page it doesn't have to be documented)

## Submitting a PR

1. Create a GitHub account if you do not have one yet: [Join GitHub](https://github.com/join).
2. Fork the public GitHub repo: [https://github.com/Azure/azure-osconfig](https://github.com/Azure/azure-osconfig). [Learn more about forking a repo](https://docs.github.com/en/github/getting-started-with-github/fork-a-repo).
3. Clone the forked repo. Optionally create a new branch to keep your changes isolated from the `dev` branch. By forking and cloning the public GitHub repo, a copy of repo will be created in your GitHub account and a local copy will be locally created in your clone. Use this local copy to make modifications.
4. Commit the changes locally and push to your fork.
5. When the source changes are ready, manually run [pre-commit](https://pre-commit.com/) with the following command:
```bash
python3 -m pre_commit run --all-files
```
6. From your fork, create a PR that targets the `dev` branch. [Learn more about pull request](https://docs.github.com/en/desktop/contributing-and-collaborating-using-github-desktop/creating-an-issue-or-pull-request#creating-a-pull-request).
7. The PR triggers a series of GitHub actions that will validate the new submitted changes.

The OSConfig Core team will respond to a PR that passes all checks in 3 business days.

# Contact

You may contact the OSConfig Core team at [osconfigcore@microsoft.com](mailto:osconfigcore@microsoft.com) to ask questions about OSConfig, to report bugs, to suggest new features, or inquire about any other OSConfig related topic.
