// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <memory>
#include <CommonUtils.h>
#include <Firewall.h>
#include <Mmi.h>
#include <vector>
#include <string>

using namespace std;
class FirewallObjectTest : public FirewallObjectBase
{
public:
    std::vector <std::string> testTableStrings;
    unsigned int runCommandCount = 0;
    unsigned int utilityCount = 0;
    int DetectUtility(string utility);
    void GetTable(string tableName, string &tableString);
    void GetAllTables(vector<string> tableNames, vector<pair<string, string>>& allTableStrings);
    FirewallObjectTest(unsigned int maxPayloadSizeBytes);
    ~FirewallObjectTest();
};

vector<string> g_testTableStrings =
{
    R"""(Chain INPUT (policy ACCEPT 705 packets, 76237 bytes)
    num   pkts bytes target     prot opt in     out     source               destination
    1        0     0 DROP       all  --  *      *       3.3.3.3              0.0.0.0/0           )""",
    R"""(Chain PREROUTING (policy ACCEPT 0 packets, 0 bytes)
    num   pkts bytes target     prot opt in     out     source               destination)""",
    R"""(Chain PREROUTING (policy ACCEPT 0 packets, 0 bytes)
    num   pkts bytes target     prot opt in     out     source               destination

    Chain OUTPUT (policy ACCEPT 0 packets, 0 bytes)
    num   pkts bytes target     prot opt in     out     source               destination)""",
    R"""()""",
    R"""()"""
};

FirewallObjectTest::FirewallObjectTest(unsigned int maxPayloadSizeBytes)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
}

FirewallObjectTest::~FirewallObjectTest()
{
    ClearTableObjects();
}

int  FirewallObjectTest::DetectUtility(string utility)
{
    const int commandSuccessExitCode = 0;
    const int commandNotFoundExitCode = 127;
    const int otherExitcode = 3;
    const string iptablesUtility = "iptables";

    int status = utilityStatusCodeUnknown;
    int testExitCodes[] = {otherExitcode, commandSuccessExitCode, commandNotFoundExitCode};
    if (utility == iptablesUtility)
    {
        int tempStatus = testExitCodes[utilityCount++ % 3];
        if (tempStatus == commandSuccessExitCode)
        {
            status = utilityStatusCodeInstalled;
        }
        else if (tempStatus == commandNotFoundExitCode)
        {
            status = utilityStatusCodeNotInstalled;
        }
    }

    return status;
}

void FirewallObjectTest::GetTable(string tableName, string& tableString)
{
    UNUSED(tableName);
    tableString = "";
    char* output = nullptr;
    unsigned int size = testTableStrings.size();
    string currentTable = testTableStrings[(runCommandCount++) % size];
    if (!currentTable.empty())
    {
        size_t tableSize = (currentTable.length() + 1) * sizeof(currentTable[0]);
        output = new (std::nothrow) char[tableSize];
        if (nullptr != output)
        {
            std::fill(output, output + tableSize, 0);
            std::memcpy(output, currentTable.c_str(), currentTable.length());
        }
    }

    tableString = (output != nullptr) ? string(output) : "";
    if (output != nullptr)
    {
        free(output);
    }
}

void FirewallObjectTest::GetAllTables(vector<string> tableNames, vector<pair<string, string>>& allTableStrings)
{
    for (unsigned int i = 0; i < tableNames.size(); i++)
    {
        string tableString = "";
        GetTable(tableNames[i], tableString);
        if (!tableString.empty())
        {
            allTableStrings.push_back(make_pair(tableNames[i], tableString));
        }
    }
}

TEST (FirewallTests, DetectUtility)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    string utility = "iptables", otherUtility = "nftables";
    for (int i = 0; i < 6; i++)
    {
        ASSERT_TRUE(testModule.DetectUtility(utility) == (i % 3));
    }

    for (int i = 0; i < 6; i++)
    {
        ASSERT_TRUE(testModule.DetectUtility(otherUtility) == 0);
    }
}

TEST (FirewallTests, GetTable)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    testModule.testTableStrings =
    {
        R"""(abc)""",
        R"""(Chain INPUT (policy ACCEPT 180K packets, 24M bytes)
        num   pkts bytes target     prot opt in     out     source               destination
        1        0     0 ACCEPT     all  --  *      *       1.1.1.1              0.0.0.0/0           )""",
        R"""()"""
    };

    string testOutputString = "";
    for (unsigned int i = 0; i < testModule.testTableStrings.size(); i++)
    {
        testModule.GetTable("", testOutputString);
        ASSERT_TRUE(testOutputString == testModule.testTableStrings[i]);
    }
}

TEST (FirewallTests, GetAllTables)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    const vector<string> testTableNames = {"filter", "nat", "mangle", "raw", "security"};
    testModule.testTableStrings = g_testTableStrings;
    vector<pair<string, string>> tableStrings;
    testModule.GetAllTables(testTableNames, tableStrings);

    ASSERT_TRUE(tableStrings.size() == 3);
    for (unsigned int i = 0; i < 3; i++)
    {
        ASSERT_TRUE(tableStrings[i].first == testTableNames[i]);
        ASSERT_TRUE(tableStrings[i].second == testModule.testTableStrings[i]);
    }
}

TEST (FirewallTests, ParseRule)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    vector<string> testRuleStrings
    {
        R"""(     11   ACCEPT   tcp  --  0.0.0.0/0            0.0.0.0/0           tcp dpt:8044 state NEW  )""",
        R"""( 1        0     0 ACCEPT     all  --  in      *       1.1.1.1        )""",
        R"""(2        0     12     all  --  *      *       198.1.1.1        )""",
        R"""(3        0     0 ACCEPT     tcp  --      *       77.66.55.44          0.0.0.0/0            tcp dpt:22)""",
        R"""(abc)"""
        R"""(

          )""",
        R"""(  )""",
        R"""()"""
    };

    for (unsigned int i = 0; i < testRuleStrings.size(); i++)
    {
        Rule* rule = testModule.ParseRule(testRuleStrings[i]);
        ASSERT_TRUE(rule == nullptr);
    }

    vector<string> testStrings =
    {
        R"""(   1        0     0 DROP     tcp  --  *      *       203.0.113.0/24       0.0.0.0/0            tcp dpt:22 ctstate NEW,ESTABLISHED )""",
        R"""(2       64 63884 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp spt:22 ctstate ESTABLISHED)""",
        R"""(   3        0     0 ACCEPT     tcp  --  *      *       77.66.55.44          0.0.0.0/0            tcp dpt:22)""",
        R"""(4        0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            MAC 00:E0:4C:F1:41:6B tcp dpt:22 )""",
        R"""(5        0     0 REJECT     tcp  --  *      *      !222.111.111.222      0.0.0.0/0            tcp dpt:23 reject-with icmp-port-unreachable  )""",
        R"""( 6        0     0 ACCEPT     all  --  *      eth1    1.1.1.0/24            0.0.0.0/0 )""",
        R"""( 234     9494 1795K ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp spt:22 ctstate ESTABLISHED)""",
        R"""(100       70 16854 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp dpt:22 ctstate NEW,ESTABLISHED)""",
        R"""(16        0     0 MASQUERADE  all  --  *      eth0    0.0.0.0/0            0.0.0.0/0 )"""
    };

    for (unsigned int i = 0; i < testStrings.size(); i++)
    {
        Rule* rule = testModule.ParseRule(testStrings[i]);
        ASSERT_TRUE(rule != nullptr);
        free(rule);
    }

    string newTestString = R"""(  123       0     0 REJECT     tcp  --  eth0   *       1.1.1.1              2.2.2.2              tcp dpt:3306 state NEW,ESTABLISHED reject-with icmp-port-unreachable)""";
    Rule* rule = testModule.ParseRule(newTestString);
    ASSERT_TRUE(rule != nullptr);
    ASSERT_TRUE(rule->GetRuleNum() == 123);
    ASSERT_TRUE(rule->GetTarget() == "REJECT");
    ASSERT_TRUE(rule->GetProtocol() == "tcp");
    ASSERT_TRUE(rule->GetInInterface() == "eth0");
    ASSERT_TRUE(rule->GetOutInterface() == "*");
    ASSERT_TRUE(rule->GetSource() == "1.1.1.1");
    ASSERT_TRUE(rule->GetDestination() == "2.2.2.2");
    ASSERT_TRUE(rule->GetRawOptions() == "tcp dpt:3306 state NEW,ESTABLISHED reject-with icmp-port-unreachable");
    free(rule);
}

TEST (FirewallTests, ParseChain)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    vector<string> testInvalidStrings =
    {
        R"""(Chain INPUT ( policy ACCEPT 484 packets, 144K bytes)
        num   pkts bytes target     prot opt in     out     source               destination         
        1        0     0 ACCEPT     tcp  --  eth0   *       0.0.0.0/0            0.0.0.0/0            tcp dpt:80 state NEW,ESTABLISHED
        2        0     0 ACCEPT     icmp --  *      *       1.1.1.1/24           0.0.0.0/0            icmptype 8)""",
        R"""(16        0     0 MASQUERADE  all  --  *      eth0    0.0.0.0/0            0.0.0.0/0 )""",
        R"""(   OUTPUT (policy ACCEPT 38 packets, 3134 bytes)
        num   pkts bytes target     prot opt in     out     source               destination         
        1     4289  362K ACCEPT     all  --  *      lo      0.0.0.0/0            0.0.0.0/0           )""",
        R"""( Chain invalidUserChain (0 ref
        num   pkts bytes target     prot opt in     out     source               destination         
        1        0     0 ACCEPT       all  --  *      *       3.3.3.3              5.5.5.5 

        )""",
        R"""( abc)""",
        R"""( 
        
          )""",
        R"""(  )""",
        R"""()"""
    };

    for (unsigned int i = 0; i < testInvalidStrings.size(); i++)
    {
        Chain* chain = testModule.ParseChain(testInvalidStrings[i]);
        ASSERT_TRUE(chain == nullptr);
    }

    vector<string> testValidChainStrings =
    {
        R"""( Chain INPUT (policy DROP 0 packets, 0 bytes)
        num   pkts bytes target     prot opt in     out     source               destination         
        1     4405  371K ACCEPT     all  --  lo     *       0.0.0.0/0            0.0.0.0/0           
        2     2292  451K ACCEPT     all  --  *      *       0.0.0.0/0            0.0.0.0/0            ctstate RELATED,ESTABLISHED
        3        0     0 DROP       all  --  *      *       0.0.0.0/0            0.0.0.0/0            ctstate INVALID
        4        0     0 DROP       all  --  *      *       203.0.113.51         0.0.0.0/0           
        5        0     0 REJECT     all  --  *      *       203.0.113.51         0.0.0.0/0            reject-with icmp-port-unreachable
        6        0     0 DROP       all  --  eth0   *       203.0.113.51         0.0.0.0/0           
        7        0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp dpt:22 ctstate NEW,ESTABLISHED
        8        0     0 ACCEPT     tcp  --  *      *       203.0.113.0/24       0.0.0.0/0            tcp dpt:22 ctstate NEW,ESTABLISHED
        9        0     0 ACCEPT     tcp  --  *      *       203.0.113.0/24       0.0.0.0/0            tcp dpt:873 ctstate NEW,ESTABLISHED
        10       0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp dpt:80 ctstate NEW,ESTABLISHED
        11       0     0 ACCEPT     tcp  --  *      *       203.0.113.0/24       0.0.0.0/0            tcp dpt:3306 ctstate NEW,ESTABLISHED     
        
        )""",
        R"""(Chain FORWARD (policy DROP 0 packets, 0 bytes)
        num   pkts bytes target     prot opt in     out     source               destination         
        1        0     0 ACCEPT     all  --  eth1   eth0    0.0.0.0/0            0.0.0.0/0    )""",
        R"""( Chain OUTPUT (policy ACCEPT 38 packets, 3134 bytes)
        num   pkts bytes target     prot opt in     out     source               destination         
        1     4289  362K ACCEPT     all  --  *      lo      0.0.0.0/0            0.0.0.0/0           
        2     2434  308K ACCEPT     all  --  *      *       0.0.0.0/0            0.0.0.0/0            ctstate ESTABLISHED
        3        0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp spt:22 ctstate ESTABLISHED
        4        0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp spt:22 ctstate ESTABLISHED
        5        0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp spt:873 ctstate ESTABLISHED
        6        0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp spt:80 ctstate ESTABLISHED
        7        0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp spt:3306 ctstate ESTABLISHED)""",
        R"""(      Chain PREROUTING (policy ACCEPT 0 packets, 0 bytes)
        num   pkts bytes target     prot opt in     out     source               destination         
        
        )""",
        R"""( Chain userChain (0 references)
        num   pkts bytes target     prot opt in     out     source               destination         
        1        0     0 DROP       all  --  *      *       3.3.3.3              5.5.5.5       
        )"""
        };

    vector<string> expectedNameValues = {"INPUT", "FORWARD", "OUTPUT", "PREROUTING", "userChain"};
    vector<string> expectedPolicyValues = {"DROP", "DROP", "ACCEPT", "ACCEPT", ""};
    vector<int> expectedRuleCounts = {11, 1, 7, 0, 1};
    for (unsigned int i = 0; i < testValidChainStrings.size(); i++)
    {
        Chain* chain = testModule.ParseChain(testValidChainStrings[i]);
        ASSERT_TRUE(chain != nullptr);
        ASSERT_TRUE(chain->GetChainName() == expectedNameValues[i]);
        ASSERT_TRUE(chain->GetChainPolicy() == expectedPolicyValues[i]);
        ASSERT_TRUE(chain->GetRuleCount() == expectedRuleCounts[i]);
        if (chain != nullptr)
        {
            free(chain);
        }
    }

    // Chains contain invalid rules, skip them
    vector<string> partialValidChains =
    {
        R"""( Chain OUTPUT (policy ACCEPT 38 packets, 3134 bytes)
        num   pkts bytes target     prot opt in     out     source               destination         
        xxx     4289  362K ACCEPT     all  --  *      lo      0.0.0.0/0            0.0.0.0/0           
        2     2434  308K ACCEPT     all  --  *      *       0.0.0.0/0            0.0.0.0/0            ctstate ESTABLISHED   )""",
        R"""(Chain INPUT (policy ACCEPT 1166 packets, 142K bytes)
        num   pkts bytes target     prot opt in     out     source               destination         
        1        0     0 LOG        all  --     *       0.0.0.0/0            0.0.0.0/0            LOG flags 0 level 4 prefix "IPtables dropped packets:"
        2      505 39708ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            multiport dports 22,80,443)"""
    };

    expectedNameValues = {"OUTPUT", "INPUT"};
    expectedPolicyValues = {"ACCEPT", "ACCEPT"};
    expectedRuleCounts = {1, 0};
    for (unsigned int i = 0; i < partialValidChains.size(); i++)
    {
        Chain* chain = testModule.ParseChain(partialValidChains[i]);
        ASSERT_TRUE(chain != nullptr);
        ASSERT_TRUE(chain->GetChainName() == expectedNameValues[i]);
        ASSERT_TRUE(chain->GetChainPolicy() == expectedPolicyValues[i]);
        ASSERT_TRUE(chain->GetRuleCount() == expectedRuleCounts[i]);
        if (chain != nullptr)
        {
            free(chain);
        }
    }
}

TEST (FirewallTests, ParseTable)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    vector<string> testTableStrings =
    {
        R"""(Chain INPUT (policy ACCEPT 353 packets, 23920 bytes)
        num   pkts bytes target     prot opt in     out     source               destination
        1        0     0 ACCEPT     all  --  *      *       1.1.1.1              0.0.0.0/0
        2        0     0 DROP       all  --  *      *       202.0.222.22         0.0.0.0/0
        3        0     0 REJECT     all  --  *      *       203.0.113.51         0.0.0.0/0            reject-with icmp-port-unreachable
        4        0     0 ACCEPT     tcp  --  *      *       203.0.111.0/24       0.0.0.0/0            tcp dpt:22 ctstate NEW,ESTABLISHED
        5        0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp dpt:80 ctstate NEW,ESTABLISHED

        Chain FORWARD (policy ACCEPT 0 packets, 0 bytes)
        num   pkts bytes target     prot opt in     out     source               destination
        1        0     0 ACCEPT     all  --  eth1   eth0    0.0.0.0/0            0.0.0.0/0

        Chain OUTPUT (policy ACCEPT 244 packets, 15920 bytes)
        num   pkts bytes target     prot opt in     out     source               destination
        1      162 13872 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp spt:22 ctstate ESTABLISHED
        2        0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp spt:80 ctstate ESTABLISHED

        )""",
        R"""(
            Chain INPUT (policy ACCEPT 399 packets, 26482 bytes)
        num   pkts bytes target     prot opt in     out     source               destination

        Chain FORWARD (policy ACCEPT 0 packets, 0 bytes)
        num   pkts bytes target     prot opt in     out     source               destination

        Chain OUTPUT (policy ACCEPT 401 packets, 27934 bytes)
        num   pkts bytes target     prot opt in     out     source               destination

        )""",
        R"""(
            Chain INPUT (policy ACCEPT 399 packets, 26482 bytes)
        num   pkts bytes target     prot opt in     out     source               destination         )""",
        R"""(abc123 INPUT (policy ACCEPT 399 packets, 26482 bytes)
        num   pkts bytes target     prot opt in     out     source               destination
        )""",
        R"""(Chain InvalidChain (0 referenc
        num   pkts bytes target     prot opt in     out     source               destination
        1        0     0 DROP       all  --  *      *       3.3.3.3              5.5.5.5

        )""",
        R"""(        num   pkts bytes target     prot opt in     out     source               destination
        1        0     0 ACCEPT     all  --  *      *       1.1.1.1              0.0.0.0/0
        2        0     0 DROP       all  --  *      *       202.0.222.22         0.0.0.0/0

        Chain FORWARD (policy ACCEPT 0 packets, 0 bytes)
        num   pkts bytes target     prot opt in     out     source               destination
        1        0     0 ACCEPT     all  --  eth1   eth0    0.0.0.0/0            0.0.0.0/0  )"""
    };

    vector<string> tableNames = {"filter", "mytable", "test_table", "", "TableWithInvalidChain", "TableWithOneInvalidChain"};
    vector<int> expectedChainCounts = {3, 3, 1, 0, 0, 1};
    for (unsigned int i = 0; i < testTableStrings.size(); i++)
    {
        Table* table = testModule.ParseTable(tableNames[i], testTableStrings[i]);
        ASSERT_TRUE(table != nullptr);
        ASSERT_TRUE(table->GetTableName() == tableNames[i]);
        ASSERT_TRUE(table->GetChainCount() == expectedChainCounts[i]);
        free(table);
    }

    vector<string> testInvalidTableStrings =
    {
        R"""(abc 123)""",
        R"""(

        )""",
        R"""()""",
        R"""(chain Invalid (0 references)"""
    };

    tableNames = {"invalidTable", "mytable", "test_invalid_table", ""};
    for (unsigned int i = 0; i < testInvalidTableStrings.size(); i++)
    {
        Table* table = testModule.ParseTable(tableNames[i], testInvalidTableStrings[i]);
        ASSERT_TRUE(table != nullptr);
        ASSERT_TRUE(table->GetTableName() == tableNames[i]);
        ASSERT_TRUE(table->GetChainCount() == 0);
        free(table);
    }
}

TEST (FirewallTests, AppendTable)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    Table* table0 = new Table("testTable0");
    testModule.AppendTable(table0);
    ASSERT_TRUE(testModule.GetTableCount() == 1);

    Table* table1 = new Table("testTable1");
    testModule.AppendTable(table1);
    ASSERT_TRUE(testModule.GetTableCount() == 2);
}

TEST (FirewallTests, GetFirewallState)
{
    int firewallStatuCode = firewallStateCodeUnknown;
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    // Test firewall status using only detect utility status
    // Currently testModule has no tables in it
    vector<int> expectedStatusCode = {firewallStateCodeUnknown, firewallStateCodeDisabled, firewallStateCodeDisabled};
    for (int i = 0; i < 3; i++)
    {
        firewallStatuCode = testModule.GetFirewallState();
        ASSERT_TRUE(firewallStatuCode == expectedStatusCode[i]);
    }
    string tableString =
    R"""(Chain INPUT (policy ACCEPT 0 packets, 0 bytes)
    num   pkts bytes target     prot opt in     out     source               destination  

    Chain FORWARD (policy DROP 0 packets, 0 bytes)
    num   pkts bytes target     prot opt in     out     source               destination       
     )""";

    Table* table = testModule.ParseTable("testTable", tableString);
    ASSERT_TRUE(table != nullptr);
    ASSERT_TRUE(table->GetChainCount() == 2);
    vector<Chain*> chainVector = table->GetChains();
    vector<string> expectedChainPolicies = {"ACCEPT", "DROP"};
    vector<int> expectedRuleCounts = {0, 0};
    for (unsigned int i = 0; i < chainVector.size(); i++)
    {
        ASSERT_TRUE(chainVector[i] != nullptr);
        ASSERT_TRUE(chainVector[i]->GetChainPolicy() == expectedChainPolicies[i]);
        ASSERT_TRUE(chainVector[i]->GetRuleCount() == expectedRuleCounts[i]);
    }
    testModule.AppendTable(table);
    ASSERT_TRUE(testModule.GetTableCount() == 1);

    // When utilityCount is 1, detect utility returns installed
    testModule.utilityCount = 1;
    firewallStatuCode = testModule.GetFirewallState();
    ASSERT_TRUE(firewallStatuCode == firewallStateCodeEnabled);

    tableString =
    R"""(Chain INPUT (policy ACCEPT 353 packets, 23920 bytes)
    num   pkts bytes target     prot opt in     out     source               destination         
           

    Chain FORWARD (policy ACCEPT 0 packets, 0 bytes)
    num   pkts bytes target     prot opt in     out     source               destination         
    1        0     0 ACCEPT     all  --  eth1   eth0    0.0.0.0/0            0.0.0.0/0           
    )""";
    FirewallObjectTest testModule2(maxPayloadSizeBytes);
    table = testModule2.ParseTable("filter", tableString);
    ASSERT_TRUE(table != nullptr);
    testModule2.AppendTable(table);
    ASSERT_TRUE(testModule2.GetTableCount() == 1);
    ASSERT_TRUE(table->GetChainCount() == 2);
    chainVector = table->GetChains();
    expectedChainPolicies = {"ACCEPT", "ACCEPT"};
    expectedRuleCounts = {0, 1};
    for (unsigned int i = 0; i < chainVector.size(); i++)
    {
        ASSERT_TRUE(chainVector[i] != nullptr);
        ASSERT_TRUE(chainVector[i]->GetChainPolicy() == expectedChainPolicies[i]);
        ASSERT_TRUE(chainVector[i]->GetRuleCount() == expectedRuleCounts[i]);
    }

    testModule2.utilityCount = 1;
    firewallStatuCode = testModule2.GetFirewallState();
    ASSERT_TRUE(firewallStatuCode == firewallStateCodeEnabled);

    tableString =
    R"""(Chain INPUT (policy ACCEPT 0 packets, 0 bytes)
    num   pkts bytes target     prot opt in     out     source               destination         

    Chain FORWARD (policy ACCEPT 0 packets, 0 bytes)
    num   pkts bytes target     prot opt in     out     source               destination         

    Chain OUTPUT (policy ACCEPT 0 packets, 0 bytes)
    num   pkts bytes target     prot opt in     out     source               destination             
    )""";
    FirewallObjectTest testModule3(maxPayloadSizeBytes);
    table = testModule3.ParseTable("filter", tableString);
    ASSERT_TRUE(table != nullptr);
    testModule3.AppendTable(table);
    ASSERT_TRUE(testModule3.GetTableCount() == 1);
    ASSERT_TRUE(table->GetChainCount() == 3);
    chainVector = table->GetChains();
    expectedChainPolicies = {"ACCEPT", "ACCEPT", "ACCEPT"};
    expectedRuleCounts = {0, 0, 0};

    for (unsigned int i = 0; i < chainVector.size(); i++)
    {
        ASSERT_TRUE(chainVector[i] != nullptr);
        ASSERT_TRUE(chainVector[i]->GetChainPolicy() == expectedChainPolicies[i]);
        ASSERT_TRUE(chainVector[i]->GetRuleCount() == expectedRuleCounts[i]);
    }
    testModule3.utilityCount = 1;
    firewallStatuCode = testModule2.GetFirewallState();
    ASSERT_TRUE(firewallStatuCode == firewallStateCodeDisabled); 
}

TEST (FirewallTests, RulesToString)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    vector<string> ruleStrings =
    {
        R"""(
        1     4205  371K ACCEPT     all  --  lo     *       1.1.1.2/0            0.0.0.0/0 )""",
        R"""(        2     2292  400K ACCEPT     all  --  *      *       0.0.0.0/0            0.0.0.0/0            ctstate RELATED,ESTABLISHED  )"""
    };
    const char expectedString[] =
        "1 ACCEPT all 1.1.1.2/0 0.0.0.0/0 lo *  "
        "2 ACCEPT all 0.0.0.0/0 0.0.0.0/0 * * ctstate RELATED,ESTABLISHED   ";
    vector<Rule*> testRules;
    for (unsigned int i = 0; i < ruleStrings.size(); ++i)
    {
        Rule* rule = testModule.ParseRule(ruleStrings[i]);
        if (rule != nullptr)
        {
            testRules.push_back(rule);
        }
    }
    string resultString = testModule.RulesToString(testRules);
    ASSERT_TRUE(resultString == expectedString);
}

TEST (FirewallTests, ChainsToString)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    vector<string> chainStrings =
    {
        R"""(Chain INPUT (policy ACCEPT 353 packets, 23920 bytes)
        num   pkts bytes target     prot opt in     out     source               destination         )""",
        R"""( Chain FORWARD (policy ACCEPT 0 packets, 0 bytes)
        num   pkts bytes target     prot opt in     out     source               destination
        1        0     0 ACCEPT     all  --  eth1   eth0    0.0.0.0/0            0.0.0.0/0           )"""
    };
    const char expectedString[] =
        "INPUT ACCEPT  "
        "FORWARD ACCEPT "
        "1 ACCEPT all 0.0.0.0/0 0.0.0.0/0 eth1 eth0   ";
    vector<Chain*> testChains;
    for (unsigned int i = 0; i < chainStrings.size(); ++i)
    {
        Chain* chain = testModule.ParseChain(chainStrings[i]);
        if (chain != nullptr)
        {
            testChains.push_back(chain);
        }
    }
    string resultString = testModule.ChainsToString(testChains);
    ASSERT_TRUE(resultString == expectedString);
}

TEST (FirewallTests, TablesToString)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    vector<string> tableStrings =
    {
        R"""(Chain INPUT (policy ACCEPT 353 packets, 23920 bytes)
        num   pkts bytes target     prot opt in     out     source               destination
        Chain FORWARD (policy ACCEPT 0 packets, 0 bytes)
        num   pkts bytes target     prot opt in     out     source               destination
        1        0     0 ACCEPT     all  --  eth1   eth0    0.0.0.0/0            0.0.0.0/0           )""",
        R"""(Chain PREROUTING (policy ACCEPT 0 packets, 0 bytes)
        num   pkts bytes target     prot opt in     out     source               destination          )"""
    };
    const char expectedString[] =
        "filter "
        "INPUT ACCEPT  "
        "FORWARD ACCEPT "
        "1 ACCEPT all 0.0.0.0/0 0.0.0.0/0 eth1 eth0    "
        "nat "
        "PREROUTING ACCEPT   ";
    vector<Table*> testTables;
    vector<string> tableNames = {"filter", "nat"};
    for (unsigned int i = 0; i < tableStrings.size(); ++i)
    {
        Table* table = testModule.ParseTable(tableNames[i], tableStrings[i]);
        if (table != nullptr)
        {
            testTables.push_back(table);
        }
    }
    string resultString = testModule.TablesToString(testTables);
    ASSERT_TRUE(resultString == expectedString);
}

TEST (FirewallTests, GetFingerprint)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    string tableString =
    R"""(Chain INPUT (policy ACCEPT 353 packets, 23920 bytes)
    num   pkts bytes target     prot opt in     out     source               destination


    Chain FORWARD (policy ACCEPT 0 packets, 0 bytes)
    num   pkts bytes target     prot opt in     out     source               destination
    1        0     0 ACCEPT     all  --  eth1   eth0    0.0.0.0/0            0.0.0.0/0
    )""";
    Table* table = testModule.ParseTable("filter", tableString);
    testModule.AppendTable(table);
    string fingerprint = testModule.GetFingerprint();
    unsigned int numberOfTests = 10;
    for (unsigned int i = 0; i < numberOfTests; i++)
    {
        ASSERT_TRUE(testModule.GetFingerprint() == fingerprint);
    }
}

TEST (FirewallTests, Get)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    const string testFirewallState = "firewallState";
    const string testFirewallFingerprint = "firewallFingerprint";
    const string testWrongObjectName = "abc";
    testModule.testTableStrings = g_testTableStrings;
    MMI_JSON_STRING payload;
    int payloadSizeBytes;
    int status = 0;
    vector<string> expectedStrings = {"0", "1", "2"};
    string resultString= "";
    for (unsigned int i = 0; i < 3; i++)
    {
        status = testModule.Get(nullptr, nullptr, testFirewallState.c_str(), &payload, &payloadSizeBytes);
        if (status != ENODATA)
        {
            resultString = string(payload, payloadSizeBytes);
            ASSERT_TRUE(resultString == expectedStrings[i]);
        }
    }

    // When object name is neither state nor fingerprint, return ENIVAL
    status = testModule.Get(nullptr, nullptr, testWrongObjectName.c_str(), &payload, &payloadSizeBytes);
    ASSERT_TRUE(status == EINVAL);

    status = testModule.Get(nullptr, nullptr, nullptr, &payload, &payloadSizeBytes);
    ASSERT_TRUE(status == EINVAL);
}

TEST(FirewallTests, CreateStatePayload)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    FirewallObjectTest testModule2(maxPayloadSizeBytes);
    vector<string> expectedPayload = {"0", "1", "2", "", ""};
    for (int i = 0; i < int(expectedPayload.size()); i++)
    {
        ASSERT_TRUE(testModule.CreateStatePayload(i) == expectedPayload[i]);
        ASSERT_TRUE(testModule2.CreateStatePayload(i) == expectedPayload[i]);
    }
}

TEST(FirewallTests, CreateFingerprintPayload)
{
    unsigned int maxPayloadSizeBytes = 0;
    FirewallObjectTest testModule(maxPayloadSizeBytes);
    FirewallObjectTest testModule2(maxPayloadSizeBytes);
    vector<string> testFingerprints = {"", "4bb0e1595", "@:=0e1595f66f344c1cc084e163c4352235b2accf3a1385b9eb4b3e4ca5b1d24", "4bb0e1595f66f344c1cc084e163c4352235b2accf3a1385b9eb4b3e4ca5b1d24", "5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03"};
    vector<string> expectedPayload = {"", "", "", "\"4bb0e1595f66f344c1cc084e163c4352235b2accf3a1385b9eb4b3e4ca5b1d24\"", "\"5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03\""};
    for (int i = 0; i < int(expectedPayload.size()); i++)
    {
        ASSERT_TRUE(testModule.CreateFingerprintPayload(testFingerprints[i]) == expectedPayload[i]);
        ASSERT_TRUE(testModule2.CreateFingerprintPayload(testFingerprints[i]) == expectedPayload[i]);
    }
}