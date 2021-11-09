# Introduction

Azure Device OS Configuration (OSConfig) is a modular services stack running on a Linux IoT Edge device that facilitates remote device management over Azure as well from local management authorities. OSConfig contains a PnP Agent, a Management Platform (Modules Manager) and several Management Modules. For more information on OSConfig see [OSConfig North Star Architecture](docs/architecture.md) and [OSConfig Roadmap](docs/roadmap.md).

# Code of conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

# Contributing

We welcome contributions to OSConfig. The main way of contributing to and extending OSConfig is via developing new **Management Modules**. See [OSConfig Management Modules](docs/modules.md).

Contributions to OSConfig can happen in one of the following ways:

Contribution | Where | Pull request implications
-----|-----|-----
Adding a new fully decoupled OSConfig Management Module | New /src/modules/<modulename> | The most common type of contribution to OSConfig. Pull requests that add a new module without changes in other components are the easiest ones to get approved.
Modifying an existing OSConfig Management Module | Existing /src/modules/<modulename> | For example, when upgrading the version of an existing module. Similar to previous kind of requests.
Adding new and/or changing multiple OSConfig Management Modules at once | /src/modules/ | Generally not recommended. Such pull requests may be accepted if the change is simple enough to warrant a bulk request. It is recommended to split into separate pull request for each module.
Adding a new OSConfig Management Module with changes into the OSConfig Management Platform, the OSConfig Agent or the common libraries | /src/modules/, /src/platform/, /src/agents/, /src/common/ | Generally not recommended. Such pull requests require special review and approval from the core team and may be approved on a case by case basis.
Adding a new and/or changing an existing feature of the OSConfig Management Platform, the OSConfig Agent and/or the common libraries | /src/platform/, /src/agents/, /src/common/ | Generally not recommended. Such pull requests require special review and approval from the core team and may be approved on a case by case basis. If the proposed change follows the current [OSConfig North Star Architecture](architecture.md) and [OSConfig Roadmap](roadmap.md) that increases the chance of approval.
Changing or adding to the documentation | *, /docs/ | Generally not recommended. Such pull requests require special review and approval from the core team and may be approved on a case by case basis.

Pull requests with few exceptions must contain appropriate unit-tests. We cannot allow test coverage to go down. Pull requests containing code changes without accompanying unit tests may be rejected.

Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com. When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

# Contact

You may contact the OSConfig Core team at [osconfigcore@microsoft.com](mailto:osconfigcore@microsoft.com) to ask questions about OSConfig, to report bugs, to suggest new features, or inquire about any other OSConfig related topic.