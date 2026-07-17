#!/bin/bash

# ##############################################################################
#
# Syncs Claude Code skills (.claude/skills/) from the dmod repository into the
# current module repository. Run this explicitly whenever you want the latest
# ecosystem knowledge available to Claude Code - it is not run automatically
# by CMake or by any build step, on purpose: the result is meant to be
# reviewed and committed like any other file, not regenerated silently.
#
# Only .claude/skills/ is touched. Local .claude/settings.json, hooks, or
# other skills you added under a different name are left alone.
#
# Usage:
#   ./scripts/sync-claude.sh [--dmod-dir PATH] [--ref REF]
#
# Options:
#   --dmod-dir PATH   Use a local dmod checkout instead of cloning from GitHub
#   --ref REF         Git branch/tag to clone when no --dmod-dir is given
#                      (default: develop)
#   --help            Show this help message
#
# ##############################################################################

set -e

DMOD_DIR=""
GIT_REF="develop"
DMOD_REPO_URL="https://github.com/choco-technologies/dmod.git"

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

print_error() {
    echo -e "${RED}Error: $1${NC}" >&2
}

print_success() {
    echo -e "${GREEN}$1${NC}"
}

print_help() {
    cat << EOF
Sync Claude Code skills from dmod

Usage:
  ./scripts/sync-claude.sh [--dmod-dir PATH] [--ref REF]

Options:
  --dmod-dir PATH   Use a local dmod checkout instead of cloning from GitHub
  --ref REF         Git branch/tag to clone when no --dmod-dir is given (default: develop)
  --help            Show this help message
EOF
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --dmod-dir)
            DMOD_DIR="$2"
            shift 2
            ;;
        --ref)
            GIT_REF="$2"
            shift 2
            ;;
        --help)
            print_help
            exit 0
            ;;
        *)
            print_error "Unknown parameter: $1"
            print_help
            exit 1
            ;;
    esac
done

TMP_CLONE_DIR=""

cleanup() {
    if [[ -n "${TMP_CLONE_DIR}" && -d "${TMP_CLONE_DIR}" ]]; then
        rm -rf "${TMP_CLONE_DIR}"
    fi
}
trap cleanup EXIT

if [[ -n "${DMOD_DIR}" ]]; then
    if [[ ! -d "${DMOD_DIR}" ]]; then
        print_error "--dmod-dir does not exist: ${DMOD_DIR}"
        exit 1
    fi
    SOURCE_SKILLS_DIR="${DMOD_DIR}/.claude/skills"
    echo "Using local dmod checkout: ${DMOD_DIR}"
else
    TMP_CLONE_DIR="$(mktemp -d)"
    echo "Cloning dmod (branch: ${GIT_REF}) ..."
    git clone --depth 1 --branch "${GIT_REF}" "${DMOD_REPO_URL}" "${TMP_CLONE_DIR}" > /dev/null 2>&1
    SOURCE_SKILLS_DIR="${TMP_CLONE_DIR}/.claude/skills"
fi

if [[ ! -d "${SOURCE_SKILLS_DIR}" ]]; then
    print_error "No .claude/skills directory found at ${SOURCE_SKILLS_DIR}"
    exit 1
fi

mkdir -p .claude/skills
cp -r "${SOURCE_SKILLS_DIR}/." .claude/skills/

echo ""
print_success "Synced Claude Code skills:"
for skill_dir in .claude/skills/*/; do
    [[ -d "${skill_dir}" ]] || continue
    echo "  - $(basename "${skill_dir}")"
done
echo ""
echo "Review the changes with 'git status'/'git diff' and commit them like any other file."
