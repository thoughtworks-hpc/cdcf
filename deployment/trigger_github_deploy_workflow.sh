#! /bin/sh

curl \
  -u "${1}" \
  -X POST \
  -H "Accept: application/vnd.github.v3+json" \
  https://api.github.com/repos/thoughtworks-hpc/cdcf/actions/workflows/publish_and_deploy_image.yml/dispatches \
  -d '{"ref":"yanghui_cluster_mod"}'