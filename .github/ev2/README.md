# Kusto Geneva GDS EV2 deploy assets

EV2 packages and CLI used by `.github/workflows/kusto-ev2-deploy.yml`.

## Why here

AME-tenant Kusto AKV trusts only AME-class tenants. Submitting EV2 from the
corp tenant fails with `AKV10032 Invalid issuer`. ADO pipelines in
`dev.azure.com/microsoft/OSGData` would have worked but the Project Admin perm
grant is gated behind a JIT group not in our jitaccess catalog. The
`osconfig-prod-release-pool` GitHub Actions self-hosted runner already runs in
an AME-trusted environment and is the cleanest unblock.

## Layout

- `NonProd/` — EV2 package targeting cluster `osconfiglinuxnonprod2` + Logs Account `OSConfigDev`
- `Prod/` — EV2 package targeting cluster `osconfiglinuxprod2` + Logs Account `OSConfigProd`
- `tools/ev2` — EV2 CLI v1.8.26 (Linux). Copied from
  `msazure.visualstudio.com/Azure-Express/_git/Quickstart` (`Ev2_CLI/Linux/ev2`)
- `tools/configurations/` — EV2 CLI endpoint configs (test + prod)
- `tools/ev2cli_release.yaml` — release pointer

## Permanent home

The EV2 packages also live in the SKATE repo at
`OSConfig/Kusto/Linux/ev2/` (the canonical location). This `.github/ev2/`
copy exists only because GitHub Actions runners can't easily clone the
internal SKATE repo. Keep both in sync if either changes.

## Cleanup

After Kusto migration completes, the `ev2` binary (20MB) should be removed
from this branch before merging to main — switch the workflow to download
from a storage account or `aka.ms/ev2cli` at runtime.
