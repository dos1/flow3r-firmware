"""
Generate version based on the status of the local git checkout.

By default, a plain string will be emitted to stdout. Append command with `-c`
to generate a C source instead.

See docs/badge/firmware(-development).rst for information about version naming
and the release process.
"""

import subprocess
import sys
import os


class Tag:
    def __init__(self, name, rc):
        self.name = name
        self.rc = rc

    def __repr__(self):
        return self.name


def tags_for_commit(release, commit):
    res = []
    tags = (
        subprocess.check_output(
            [
                "git",
                "tag",
                "--contains",
                commit,
            ]
        )
        .decode()
        .strip()
    )
    for tag in tags.split("\n"):
        tag = tag.strip()
        if not tag:
            continue
        if not tag.startswith("v" + release):
            continue
        if tag == "v" + release:
            res.append(Tag(tag, False))
            continue
        if tag.startswith("v" + release + "+rc"):
            res.append(Tag(tag, True))
            continue
    return res


def get_git_based_version():
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"]).decode().strip()
    branches = (
        subprocess.check_output(
            [
                "git",
                "branch",
                "--format",
                "%(refname)",
                "--contains",
                commit,
            ]
        )
        .decode()
        .strip()
    )

    release = None
    for branch in branches.split("\n"):
        branch = branch.strip()
        if not branch:
            continue
        parts = branch.split("/")
        if len(parts) != 4:
            continue

        if parts[:3] != ["refs", "heads", "release"]:
            continue
        v = parts[3]
        release = v
        break

    main_count = (
        subprocess.check_output(
            [
                "git",
                "rev-list",
                "--count",
                commit,
            ]
        )
        .decode()
        .strip()
    )

    version = None
    if release is None:
        version = f"v0-dev{main_count}"
        return version

    tags = tags_for_commit(release, commit)
    if not tags:
        return f"v{release}-dev{main_count}"

    releases = sorted([t for t in tags if not t.rc])
    candidates = sorted([t for t in tags if t.rc])

    if releases:
        return str(releases[0])
    else:
        return str(candidates[0])


fmt = None
if len(sys.argv) > 1:
    if sys.argv[1] == "-c":
        fmt = "C"

v = None
if os.environ.get('CI') is not None:
    tag = os.environ.get('CI_COMMIT_TAG')
    if tag is not None:
        # If we're building a tag, just use that as a version.
        v = tag
if v is None:
    v = get_git_based_version()

if fmt == "C":
    print('const char *st3m_version = "' + v + '";')
else:
    print(v)
