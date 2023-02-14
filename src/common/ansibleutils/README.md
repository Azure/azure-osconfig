# AnsibleUtils

Here are a few examples for how to use the *AnsibleUtils* shared library.

## Setup

Check the environment for required dependencies (e.g., *pip*, *venv*) and external Ansible Collections.

```c
AnsibleCheckDependencies(ModuleGetLog())
```

```c
// AnsibleCheckCollection("ansible.builtin", ModuleGetLog());
AnsibleCheckCollection("community.docker", ModuleGetLog());
```

## Get configuration

Get configuration using an Ansible module.

```c
AnsibleExecuteModule("ansible.builtin", "service_facts", NULL, &result, ModuleGetLog());
```

```c
AnsibleExecuteModule("community.docker", "docker_image_info", NULL, &result, ModuleGetLog());
```

## Set configuration

Set configuration using an Ansible module.

```c
AnsibleExecuteModule("ansible.builtin", "service", "name='snapd.service' state='stopped'", NULL, ModuleGetLog());
```
