// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using NUnit.Framework;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace E2eTesting
{
    [TestFixture, Category("Pmc")]
    public class PmcTests : E2ETest
    {
        private const string _componentName = "PackageManagerConfiguration";
        private const string _desiredObjectName = "desiredState";
        private const string _reportedObjectName = "state";
        private const string _packageNameToUninstall = "sl";
        private const string _packageSourceName = "mariadb";
        private const string _validVersionPattern = "[a-zA-Z\\d\\.\\+\\-\\~\\:]+";
        private const string _gpgKeyUrl = "https://mariadb.org/mariadb_release_signing_key.asc";
        private const string _gpgKeyId = "mariadb-key";
        private const string _distributionNameEnv = "E2E_OSCONFIG_DISTRIBUTION_NAME";
        private static readonly string _distribution = Environment.GetEnvironmentVariable(_distributionNameEnv);
        private static readonly string _sourceConfigPrefix = $"deb [signed-by={_gpgKeyId}] https://mirrors.gigenet.com/mariadb/repo/10.5/";
        private static readonly Dictionary<string, string> _distributionVersionToSourceConfig = new Dictionary<string, string>(){
                    {"ubuntu-18.04", $"{_sourceConfigPrefix}ubuntu bionic main"},
                    {"ubuntu-20.04", $"{_sourceConfigPrefix}ubuntu focal main"},
                    {"debian9", $"{_sourceConfigPrefix}debian stretch main"},
                };

        private string _packageSourceConfig;

        public enum ExecutionState
        {
            Unknown = 0,
            Running,
            Succeeded,
            Failed,
            TimedOut
        }

        public enum ExecutionSubState
        {
            None = 0,
            DeserializingJsonPayload,
            DeserializingDesiredState,
            DeserializingGpgKeys,
            DeserializingSources,
            DeserializingPackages,
            DownloadingGpgKeys,
            ModifyingSources,
            UpdatingPackageLists,
            InstallingPackages
        }

        public class DesiredState
        {
            public List<string> Packages { get; set; }
            public Dictionary<string, string> Sources { get; set; }
            public Dictionary<string, string> GpgKeys { get; set; }
        }

        public class State
        {
            public List<string> Packages { get; set; }
            public string PackagesFingerprint { get; set; }
            public string SourcesFingerprint { get; set; }
            public List<string> SourcesFilenames { get; set; }
            public ExecutionState ExecutionState { get; set; }
            public ExecutionSubState ExecutionSubstate { get; set; }
            public string ExecutionSubstateDetails { get; set; }
        }

        [OneTimeSetUp]
        public void CheckIfSupportedOsDistribution()
        {
            if (_distribution == null)
            {
                Assert.Ignore($"Unable to determine OS distribution. Missing environment variable: {_distributionNameEnv}");
            }
            else if (!_distributionVersionToSourceConfig.ContainsKey(_distribution))
            {
                Assert.Ignore("Unsupported OS distribution");
            }
            else
            {
                _packageSourceConfig = _distributionVersionToSourceConfig[_distribution];
            }
        }

        [Test]
        [TestCase("cowsay", true, ACK_SUCCESS, _validVersionPattern, ExecutionState.Succeeded, ExecutionSubState.None, "",
            TestName = "PmcTest_InstallAndUninstallValidPackagesAndAddSource")]
        [TestCase("cowsay", false, ACK_SUCCESS, _validVersionPattern, ExecutionState.Succeeded, ExecutionSubState.None, "",
            TestName = "PmcTest_InstallAndUninstallValidPackagesAndDeleteSource")]
        [TestCase("nonexisting", false, ACK_ERROR, "\\(failed\\)", ExecutionState.Failed, ExecutionSubState.InstallingPackages, "nonexisting",
            TestName = "PmcTest_InstallInvalidPackageAndDeleteSource")]
        public async Task PmcTest_ConfigurePackagesAndSources(string packageNameToInstall, bool packageSourceRequired, int desiredResponseAck, string versionPattern,
                            ExecutionState executionState, ExecutionSubState executionSubState, string executionSubStateDetails)
        {
            
            var desired = new DesiredState
            {
                Packages = new List<string>() { $"{_packageNameToUninstall}-", packageNameToInstall },
                Sources = new Dictionary<string, string>() {
                    { _packageSourceName, packageSourceRequired ? _packageSourceConfig : null },
                },
                GpgKeys = new Dictionary<string, string>() {
                    { _gpgKeyId, packageSourceRequired ? _gpgKeyUrl : null }
                }
            };

            await SetDesired<DesiredState>(_componentName, _desiredObjectName, desired);
            var desiredTask = await GetReported<GenericResponse<DesiredState>>(_componentName, _desiredObjectName, (GenericResponse<DesiredState> response) =>
            {
                return (response.Ac == desiredResponseAck)
                    && (response.Value.Packages.Contains(packageNameToInstall))
                    && (response.Value.Sources.ContainsKey(_packageSourceName) == packageSourceRequired);
            });

            var reported = await GetReported<State>(_componentName, _reportedObjectName, (State state) =>
            {
                return (state.SourcesFilenames.Contains($"{_packageSourceName}.list") == packageSourceRequired)
                    && (state.Packages.FirstOrDefault(x => x.StartsWith($"{packageNameToInstall}=")) != null);
            });

            var uninstalledPackagePattern = new Regex($"{_packageNameToUninstall}=\\(none\\)");
            var installedPackagePattern = new Regex($"{packageNameToInstall}={versionPattern}");
            Assert.Multiple(() =>
            {
                Assert.AreEqual(desiredResponseAck, desiredTask.Ac);
                Assert.IsNotNull(reported.Packages.FirstOrDefault(x => uninstalledPackagePattern.IsMatch(x)));
                Assert.IsNotNull(reported.Packages.FirstOrDefault(x => installedPackagePattern.IsMatch(x)));
                Assert.AreEqual(packageSourceRequired, reported.SourcesFilenames.Contains($"{_packageSourceName}.list"));
                Assert.AreEqual(executionState, reported.ExecutionState);
                Assert.AreEqual(executionSubState, reported.ExecutionSubstate);
                Assert.AreEqual(executionSubStateDetails, reported.ExecutionSubstateDetails);
            });
        }

        [Test]
        public async Task PmcTest_InvalidPackageName()
        {
            var invalidPackageName = "cowsay ; echo foo";
            var desired = new DesiredState
            {
                Packages = new List<string>() { $"{_packageNameToUninstall}-", invalidPackageName },
                Sources = new Dictionary<string, string>() {{ _packageSourceName, null }},
                GpgKeys = new Dictionary<string, string>() {{ _gpgKeyId, null }}
            };

            await SetDesired<DesiredState>(_componentName, _desiredObjectName, desired);
            var desiredTask = await GetReported<GenericResponse<DesiredState>>(_componentName, _desiredObjectName, (GenericResponse<DesiredState> response) =>
            {
                return (response.Ac == ACK_ERROR) && (response.Value.Packages.Contains(invalidPackageName));
            });

            var reported = await GetReported<State>(_componentName, _reportedObjectName, (State state) =>
            {
                return (state.ExecutionState.Equals(ExecutionState.Failed)) 
                    && (state.ExecutionSubstate.Equals(ExecutionSubState.DeserializingPackages));
            });

            Assert.Multiple(() =>
            {
                Assert.AreEqual(ACK_ERROR, desiredTask.Ac);
                Assert.AreEqual(0, reported.Packages.Count);
                Assert.AreEqual(ExecutionState.Failed, reported.ExecutionState);
                Assert.AreEqual(ExecutionSubState.DeserializingPackages, reported.ExecutionSubstate);
                Assert.AreEqual(invalidPackageName, reported.ExecutionSubstateDetails);
            });
        }

        [Test]
        public async Task PmcTest_InvalidPackageSource()
        {
            const string wrongSourceConfig = "debz https://www.example.com";

            var desired = new DesiredState
            {
                Packages = new List<string>(),
                Sources = new Dictionary<string, string>() {{ _packageSourceName, wrongSourceConfig }},
                GpgKeys = new Dictionary<string, string>() {{ _gpgKeyId, null }}
            };

            await SetDesired<DesiredState>(_componentName, _desiredObjectName, desired);
            var desiredTask = await GetReported<GenericResponse<DesiredState>>(_componentName, _desiredObjectName, (GenericResponse<DesiredState> response) =>
            {
                return (response.Ac == ACK_ERROR) && (response.Value.Sources.ContainsKey(_packageSourceName));
            });

            var reported = await GetReported<State>(_componentName, _reportedObjectName, (State state) =>
            {
                return (state.ExecutionState.Equals(ExecutionState.Failed))
                    && (state.ExecutionSubstate.Equals(ExecutionSubState.ModifyingSources));
            });

            Assert.Multiple(() =>
            {
                Assert.AreEqual(ACK_ERROR, desiredTask.Ac);
                Assert.AreEqual(ExecutionState.Failed, reported.ExecutionState);
                Assert.AreEqual(ExecutionSubState.ModifyingSources, reported.ExecutionSubstate);
                Assert.AreEqual(_packageSourceName, reported.ExecutionSubstateDetails);
            });
        }

        [Test]
        [TestCase("httpzz://wrong.com", TestName = "PmcTest_InvalidGpgUrl")]
        [TestCase("https://www.example.com", TestName = "PmcTest_InvalidGpgKey")]
        public async Task PmcTest_InvalidGpgConfiguration(string url)
        {
            var desired = new DesiredState
            {
                Packages = new List<string>(),
                Sources = new Dictionary<string, string>() {{ _packageSourceName, null }},
                GpgKeys = new Dictionary<string, string>() {{ _gpgKeyId, url } }
            };

            await SetDesired<DesiredState>(_componentName, _desiredObjectName, desired);
            var desiredTask = await GetReported<GenericResponse<DesiredState>>(_componentName, _desiredObjectName, (GenericResponse<DesiredState> response) =>
            {
                return (response.Ac == ACK_ERROR) && (response.Value.GpgKeys.ContainsKey(_gpgKeyId));
            });

            var reported = await GetReported<State>(_componentName, _reportedObjectName, (State state) =>
            {
                return (state.ExecutionState.Equals(ExecutionState.Failed))
                    && (state.ExecutionSubstate.Equals(ExecutionSubState.DownloadingGpgKeys));
            });

            Assert.Multiple(() =>
            {
                Assert.AreEqual(ACK_ERROR, desiredTask.Ac);
                Assert.AreEqual(ExecutionState.Failed, reported.ExecutionState);
                Assert.AreEqual(ExecutionSubState.DownloadingGpgKeys, reported.ExecutionSubstate);
                Assert.AreEqual(_gpgKeyId, reported.ExecutionSubstateDetails);
            });
        }
    }
}