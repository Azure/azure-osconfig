# OSConfig E2E (end-to-end) Test
This is the E2E test for OSConfig, its purpose is to perform testing against a targeted IoT-Hub device. It is meant to be used as as test adapter in order to be executed against the dotnet testing harness (using `dotnet test`). It is designed to test the reliability of modules against a target system.

These tests work by updating the Desired state of the component under test (ComponentName) and checking to make sure the reported properties have been updated as expected. This provides a framework for adding additional tests which target any Management Module.

# Environment Setup for E2E Testing
In order to make use of this E2E Test Driver you require two nodes, one to run to tests and another running OSConfig (target). There is also an IoT-Hub required in order for both the test and target to communicate with.

# Prerequisites
 - Iot-Hub Connection String (iothubowner policy from Iot-Hub)
 - Device-Id (the target devices name)
 - OSConfig running on the target device

# System Prerequisites - e2e test Driver
 - DotNet Core 5.x - [Install .NET on Windows, Linux, and macOS](https://docs.microsoft.com/dotnet/core/install/)

# Running the E2E Tests
The test app requires two environment variables to be present:
 * `E2E_OSCONFIG_IOTHUB_CONNSTR` - Iot-Hub Connection String (iothubowner policy from Iot-Hub)
 * `E2E_OSCONFIG_DEVICE_ID` - Device-Id (the target devices name)

The tests are driven by NUnit, so simply running `dotnet test` on the test app will run the e2e tests.

# Creating New E2E Tests
In order to create new tests targeting a particular module you simply need to create a new c# class which derives from `E2eTest`. The `E2eTest` provides methods to get the Twin and update the Twin so all the tests need to perform are creating the necessary desired state and provide the expected reported state which we can test against. See the `CommandRunnerTests.cs` for a good example.