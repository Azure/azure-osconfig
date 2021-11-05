# MSFT Internal Documentation
This documentation is specific to MSFT employees.
# Build and Testing OSConfig
There are many ways to build and test OSConfig, we have pre-made containers but if you want to setup your own environment, refer to [Manual development environment setup](../README.md#prerequisites).

## VSCode Development Container Environment
The OSConfig project supports VSCode's [Development Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers).
### Requirements:
 * [Visual Studio Code](https://code.visualstudio.com/)
 * [Docker](https://www.docker.com/products/docker-desktop) (Windows Users - Enable WSL2 first -> `wsl --install`, more [info](https://docs.microsoft.com/en-us/windows/wsl/install-win10))
 * [Remote - Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) - VS Code Plugin
 * Azure CLI (Windows users, perform this in your Linux WSL instance) - see [https://aka.ms/InstallAzCLI](https://aka.ms/InstallAzCLI):

 ### Log into OSConfig ACR
 Perform the following operations to login to the osconfig container repository:
 * `az login`
 * `az acr login --name osconfig`

 You shoudl receive a `Login Succeeded` message meaning we are good to go. We can now launch VSCode and it will prompt you to open the project in a Development Container. VSCode will re-open and provision the environment for you automatically. The first time will take some time as the docker image is retreived and the develoipment container is built.

![Starting Dev Container](https://code.visualstudio.com/assets/docs/remote/containers/dev-container-progress.png)

The Dev Container environment also includes the following pre-configured plugins to ease the development experiance:
 * C/C++ Tools
 * CMake Tools

### Making changes in the dev container
The projects root is mounted as a docker volume in order for the changes in the dev container to be reflected on the host, meaning all changes are performed on the host machine and git can be used on the host/guest (dev container) interchangeably.
