# PlatformIO pre-build script: inject the firmware version as PLANE_RADAR_VERSION.
# Derived from git (tag/commit/dirty) so the Device page and serial log can show
# exactly which build is running. Falls back to "unknown" outside a git checkout.

Import("env")

import subprocess


def git_version():
    try:
        return (
            subprocess.check_output(
                ["git", "describe", "--tags", "--always", "--dirty"],
                stderr=subprocess.DEVNULL,
            )
            .decode()
            .strip()
        )
    except Exception:
        return "unknown"


version = git_version()
print(f"Firmware version: {version}")
env.Append(CPPDEFINES=[("PLANE_RADAR_VERSION", env.StringifyMacro(version))])
