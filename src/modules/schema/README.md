# Validating a Module Interface Model (MIM) JSON

[`jsonschema`](https://github.com/Julian/jsonschema) can be run to validate a MIM JSON against the [mim.schema.json](./mim.schema.json) via the command line.

`jsonschema` is available on [PyPI](https://pypi.org/project/jsonschema/). You can install using [pip](https://pip.pypa.io/en/stable/):
```bash
$ pip install jsonschema
```

To validate a MIM JSON, run the following command:

```bash
$ jsonschema --instance my-new-mim.json mim.schema.json
```
