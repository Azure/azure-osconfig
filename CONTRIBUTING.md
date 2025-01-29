# Introduction

Azure Device OS Configuration (OSConfig) is a modular configuration stack for Linux Edge devices. OSConfig supports multi-authority device management over Azure and Azure Portal/CLI (via Azure PnP, IoT Hub, Azure Policy), GitOps, as well as local management (such as from Out Of Box Experience, OOBE). For more information on OSConfig see [OSConfig North Star Architecture](docs/architecture.md).

# Code of conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

# Contributing

We welcome contributions to OSConfig. The main way of contributing to and extending OSConfig is via developing new **Management Modules**. For more information see [OSConfig Management Modules](docs/modules.md).
Pull requests with few exceptions must contain appropriate unit-tests. We cannot allow test coverage to go down. Pull requests containing code changes without accompanying unit tests may be rejected.
Pull requests needs to be formatted according to .pre-commit-config.yaml. Each pull request is checked by [Forrmating Tests](https://github.com/Azure/azure-osconfig/blob/dev/.github/workflows/formatting.yml).

Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com. When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

## Submitting a PR

1. Create a GitHub account if you do not have one yet: [Join GitHub](https://github.com/join).
2. Fork the public GitHub repo: [https://github.com/Azure/azure-osconfig](https://github.com/Azure/azure-osconfig). [Learn more about forking a repo](https://docs.github.com/en/github/getting-started-with-github/fork-a-repo).
3. Clone the forked repo. Optionally create a new branch to keep your changes isolated from the `dev` branch. By forking and cloning the public GitHub repo, a copy of repo will be created in your GitHub account and a local copy will be locally created in your clone. Use this local copy to make modifications.
4. Commit the changes locally and push to your fork.
6. From your fork, create a PR that targets the `dev` branch. [Learn more about pull request](https://docs.github.com/en/desktop/contributing-and-collaborating-using-github-desktop/creating-an-issue-or-pull-request#creating-a-pull-request).
7. The PR triggers a series of GitHub actions that will validate the new submitted changes.

The OSConfig Core team will respond to a PR that passes all checks in 3 business days.

### Run code Formatting locally for PR

Azure Device OS Configuration (OSConfig) uses [pre-commit](https://pre-commit.com/) configured in .pre-commit-config.yaml
To set it up as git pre-commit hook:

#### 0. Install python

##### Windows

```cmd
python
```
If python is not installed this will open browser and with Microsoft Store, where you can install python3.
Then go to step 1. Linux & Windows

##### Linux

```bash
sudo apt-get install python3
```

##### 1. Linux & Windows

1. Install python pre-commit-hook package

```cmd
pip3 install pre_commit
```

2. Install .git/pre-commit hook

```cmd
python3 -m pre_commit install
```

This will install allow pre-commit to run locally on each commit.

To run pre-commit manually you can always

```
python3 -m pre_commit run --all-files
```

# Contact

You may contact the OSConfig Core team at [osconfigcore@microsoft.com](mailto:osconfigcore@microsoft.com) to ask questions about OSConfig, to report bugs, to suggest new features, or inquire about any other OSConfig related topic.
