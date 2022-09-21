// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace E2eTesting
{
    [TestFixture, Category("Tpm")]
    public class TpmTests : E2ETest
    {
        private static readonly string _componentName = "Tpm";

        private static readonly Regex _tpmVersionPattern = new Regex(@"((\d+)\.)?((\d+))");
        private static readonly Regex _tpmManufacturerPattern = new Regex(@"^[A-Za-z]([0-9a-zA-Z\s]+)?[^\s]$");
        private static readonly Regex _tpmStatusPattern = new Regex(@"[0-2]");

        public enum TpmStatusCode
        {
            Unknown = 0,
            TpmDetected,
            TpmNotDetected
        }

        public class Tpm
        {
            public TpmStatusCode TpmStatus { get; set; }
            public string TpmVersion { get; set; }
            public string TpmManufacturer { get; set; }
        }

        [Test]
        public void TpmTest_Get()
        {
            Tpm reported = GetReported<Tpm>(_componentName);

            Assert.Multiple(() =>
            {
                RegexAssert.IsMatch(_tpmStatusPattern, reported.TpmStatus);

                if (!string.IsNullOrEmpty(reported.TpmVersion))
                {
                    RegexAssert.IsMatch(_tpmVersionPattern, reported.TpmVersion);
                }

                if (!string.IsNullOrEmpty(reported.TpmManufacturer))
                {
                    RegexAssert.IsMatch(_tpmManufacturerPattern, reported.TpmManufacturer);
                }

            });
        }
    }
}