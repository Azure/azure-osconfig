# Introduction

Kompli is a modular security compliance stack for Linux devices. Kompli supports multi-authority device management over Azure and Azure Portal/CLI (via Azure Policy), GitOps, as well as local management. For more information on Kompli see [Architecture](docs/architecture.md).

# Code of conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

# Contributing

We welcome contributions to Kompli. The main way of contributing to and extending Kompli is via developing new procedure [Procedures](src/modules/complianceengine/src/lib/procedures)

Pull requests with few exceptions must contain appropriate unit-tests. We cannot allow test coverage to go down. Pull requests containing code changes without accompanying unit tests may be rejected.
Pull requests need to be formatted according to `.pre-commit-config.yaml`. Each pull request is checked by [Formatting Tests](https://github.com/microsoft/kompli/blob/main/.github/workflows/formatting.yml).

Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com. When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

# Coding style
The coding style is described in a separate [style document](docs/style.md).

## Submitting a PR

1. Create a GitHub account if you do not have one yet: [Join GitHub](https://github.com/join).
2. Fork the public GitHub repo: [https://github.com/microsoft/kompli](https://github.com/microsoft/kompli). [Learn more about forking a repo](https://docs.github.com/en/github/getting-started-with-github/fork-a-repo).
3. Clone the forked repo. Optionally create a new branch to keep your changes isolated from the `main` branch. By forking and cloning the public GitHub repo, a copy of repo will be created in your GitHub account and a local copy will be locally created in your clone. Use this local copy to make modifications.
4. Commit the changes locally and push to your fork.
5. When the source changes are ready, manually run [pre-commit](https://pre-commit.com/) with the following command:
```bash
python3 -m pre_commit run --all-files
```
6. From your fork, create a PR that targets the `main` branch. [Learn more about pull requests](https://docs.github.com/en/desktop/contributing-and-collaborating-using-github-desktop/creating-an-issue-or-pull-request#creating-a-pull-request).
7. The PR triggers a series of GitHub actions that will validate the new submitted changes.

The Kompli team will respond to a PR that passes all checks in 3 business days.

# Contact

You may contact the Kompli team at [kompli@microsoft.com](mailto:kompli@microsoft.com) to ask questions about Kompli, to report bugs, to suggest new features, or inquire about any other Kompli-related topic.
