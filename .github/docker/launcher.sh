#!/usr/bin/env bash

# -e: exit when any command fails
# -x: all executed commands are printed to the terminal
# -o pipefail: prevents errors in a pipeline from being masked
set -exo pipefail

export DOCKER_WORKSPACE_DIR="/root/$PROJECT_NAME"

# Do not share the same workspace
# GITHUB_WORKSPACE is set by actions/checkout@v3
cp -rf $GITHUB_WORKSPACE $JOB_WORKSPACE_DIR

docker run \
           --name $CONTAINER_NAME \
           --memory-swap -1 \
           --env WORKSPACE_DIR=$DOCKER_WORKSPACE_DIR \
           --env DOCKER_IMAGE=$DOCKER_IMAGE \
           --env SCRIPT_NAME=$SCRIPT_NAME \
           --env-file .github/docker/docker.env \
           -v "${JOB_WORKSPACE_DIR}:${DOCKER_WORKSPACE_DIR}" \
           ${DOCKER_IMAGE} \
           /bin/bash -c "${DOCKER_WORKSPACE_DIR}/.github/docker/script.sh"

exit 0
