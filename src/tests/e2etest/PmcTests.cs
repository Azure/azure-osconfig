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
            public List<string> packages { get; set; }
            public Dictionary<string, string> sources { get; set; }
            public Dictionary<string, string> gpgKeys { get; set; }
        }

        public class State
        {
            public List<string> packages { get; set; }
            public string packagesFingerprint { get; set; }
            public string sourcesFingerprint { get; set; }
            public List<string> sourcesFilenames { get; set; }
            public ExecutionState executionState { get; set; }
            public ExecutionSubState executionSubstate { get; set; }
            public string executionSubstateDetails { get; set; }
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
        [TestCase("cowsay", true, true, _validVersionPattern, ExecutionState.Succeeded, ExecutionSubState.None, "",
            TestName = "PmcTest_InstallAndUninstallValidPackagesAndAddSource")]
        [TestCase("cowsay", false, true, _validVersionPattern, ExecutionState.Succeeded, ExecutionSubState.None, "",
            TestName = "PmcTest_InstallAndUninstallValidPackagesAndDeleteSource")]
        [TestCase("nonexisting", false, false, "\\(failed\\)", ExecutionState.Failed, ExecutionSubState.InstallingPackages, "nonexisting",
            TestName = "PmcTest_InstallInvalidPackageAndDeleteSource")]
        public void PmcTest_ConfigurePackagesAndSources(string packageNameToInstall, bool packageSourceRequired, bool desiredResponse, string versionPattern,
                                                        ExecutionState executionState, ExecutionSubState executionSubState, string executionSubStateDetails)
        {

            var desired = new DesiredState
            {
                packages = new List<string>() { $"{_packageNameToUninstall}-", packageNameToInstall },
                sources = new Dictionary<string, string>() {
                    { _packageSourceName, packageSourceRequired ? _packageSourceConfig : null },
                },
                gpgKeys = new Dictionary<string, string>() {
                    { _gpgKeyId, packageSourceRequired ? _gpgKeyUrl : null }
                }
            };

            Assert.AreEqual(desiredResponse, SetDesired<DesiredState>(_componentName, _desiredObjectName, desired));

            var reported = GetReported<State>(_componentName, _reportedObjectName, (State state) =>
            {
                return (state.sourcesFilenames.Contains($"{_packageSourceName}.list") == packageSourceRequired)
                    && (state.packages.FirstOrDefault(x => x.StartsWith($"{packageNameToInstall}=")) != null);
            });

            var uninstalledPackagePattern = new Regex($"{_packageNameToUninstall}=\\(none\\)");
            var installedPackagePattern = new Regex($"{packageNameToInstall}={versionPattern}");
            Assert.Multiple(() =>
            {
                Assert.IsNotNull(reported.packages.FirstOrDefault(x => uninstalledPackagePattern.IsMatch(x)));
                Assert.IsNotNull(reported.packages.FirstOrDefault(x => installedPackagePattern.IsMatch(x)));
                Assert.AreEqual(packageSourceRequired, reported.sourcesFilenames.Contains($"{_packageSourceName}.list"));
                Assert.AreEqual(executionState, reported.executionState);
                Assert.AreEqual(executionSubState, reported.executionSubstate);
                Assert.AreEqual(executionSubStateDetails, reported.executionSubstateDetails);
            });
        }

        [Test]
        public void PmcTest_InvalidPackageName()
        {
            var invalidPackageName = "cowsay ; echo foo";
            var desired = new DesiredState
            {
                packages = new List<string>() { $"{_packageNameToUninstall}-", invalidPackageName },
                sources = new Dictionary<string, string>() { { _packageSourceName, null } },
                gpgKeys = new Dictionary<string, string>() { { _gpgKeyId, null } }
            };

            Assert.IsFalse(SetDesired<DesiredState>(_componentName, _desiredObjectName, desired));
            var reported = GetReported<State>(_componentName, _reportedObjectName, (State state) =>
            {
                return (state.executionState.Equals(ExecutionState.Failed))
                    && (state.executionSubstate.Equals(ExecutionSubState.DeserializingPackages));
            });

            Assert.Multiple(() =>
            {
                Assert.AreEqual(0, reported.packages.Count);
                Assert.AreEqual(ExecutionState.Failed, reported.executionState);
                Assert.AreEqual(ExecutionSubState.DeserializingPackages, reported.executionSubstate);
                Assert.AreEqual(invalidPackageName, reported.executionSubstateDetails);
            });
        }

        [Test]
        public void PmcTest_InvalidPackageSource()
        {
            const string wrongSourceConfig = "debz https://www.example.com";

            var desired = new DesiredState
            {
                packages = new List<string>(),
                sources = new Dictionary<string, string>() { { _packageSourceName, wrongSourceConfig } },
                gpgKeys = new Dictionary<string, string>() { { _gpgKeyId, null } }
            };

            Assert.IsFalse(SetDesired<DesiredState>(_componentName, _desiredObjectName, desired));
            var reported = GetReported<State>(_componentName, _reportedObjectName, (State state) =>
            {
                return (state.executionState.Equals(ExecutionState.Failed))
                    && (state.executionSubstate.Equals(ExecutionSubState.ModifyingSources));
            });

            Assert.Multiple(() =>
            {
                Assert.AreEqual(ExecutionState.Failed, reported.executionState);
                Assert.AreEqual(ExecutionSubState.ModifyingSources, reported.executionSubstate);
                Assert.AreEqual(_packageSourceName, reported.executionSubstateDetails);
            });
        }

        [Test]
        [TestCase("httpzz://wrong.com", TestName = "PmcTest_InvalidGpgUrl")]
        [TestCase("https://www.example.com", TestName = "PmcTest_InvalidGpgKey")]
        public void PmcTest_InvalidGpgConfiguration(string url)
        {
            var desired = new DesiredState
            {
                packages = new List<string>(),
                sources = new Dictionary<string, string>() { { _packageSourceName, null } },
                gpgKeys = new Dictionary<string, string>() { { _gpgKeyId, url } }
            };

            Assert.IsFalse(SetDesired<DesiredState>(_componentName, _desiredObjectName, desired));
            var reported = GetReported<State>(_componentName, _reportedObjectName, (State state) =>
            {
                return (state.executionState.Equals(ExecutionState.Failed))
                    && (state.executionSubstate.Equals(ExecutionSubState.DownloadingGpgKeys));
            });

            Assert.Multiple(() =>
            {
                Assert.AreEqual(ExecutionState.Failed, reported.executionState);
                Assert.AreEqual(ExecutionSubState.DownloadingGpgKeys, reported.executionSubstate);
                Assert.AreEqual(_gpgKeyId, reported.executionSubstateDetails);
            });
        }
    }
}