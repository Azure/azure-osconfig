// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using Microsoft.Azure.Devices;
using System.Text.Json;
using System;
using System.Threading.Tasks;
using System.Text.RegularExpressions;

namespace e2etesting
{
    public class TpmTests : E2eTest
    {
        readonly string ComponentName = "Tpm";
        public enum TpmStatusCode
        {
            Unknown = 0,
            TpmDetected,
            TpmNotDetected
        }
        public partial class Tpm
        {
            public string TpmVersion { get; set; }
            public string TpmManufacturer { get; set; }
            public TpmStatusCode TpmStatus { get; set; }
        }
        public partial class ExpectedTpmPattern
        {
            public Regex TpmVersionPattern { get; set; }
            public Regex TpmManufacturerPattern { get; set; }
            public Regex TpmStatusPattern { get; set; }
        }

        [Test]
        public void TpmTest()
        {
            var expectedTpmPattern = new ExpectedTpmPattern
            {
                TpmVersionPattern = new Regex(@"((\d+)\.)?((\d+))"),
                TpmManufacturerPattern = new Regex(@"^[A-Za-z]([0-9a-zA-Z\s]+)?[^\s]$"),
                TpmStatusPattern = new Regex(@"[0-2]")
            };
            var deserializedReportedObject = JsonSerializer.Deserialize<Tpm>(GetTwin().Properties.Reported[ComponentName].ToString());

            if ((GetTwin().ConnectionState == DeviceConnectionState.Disconnected) || (GetTwin().Status == DeviceStatus.Disabled))
            {
                Assert.Fail("Module is disconnected or is disabled");
            }

            Assert.True(IsRegexMatch(expectedTpmPattern.TpmStatusPattern, deserializedReportedObject.TpmStatus));
            Assert.True(String.IsNullOrEmpty(deserializedReportedObject.TpmVersion) || IsRegexMatch(expectedTpmPattern.TpmVersionPattern, deserializedReportedObject.TpmVersion));
            Assert.True(String.IsNullOrEmpty(deserializedReportedObject.TpmManufacturer) || IsRegexMatch(expectedTpmPattern.TpmManufacturerPattern, deserializedReportedObject.TpmManufacturer));
        }
    }
}