#!/usr/bin/env bash

set -e

PATCH_FILE=tune-for-single-static-gpu.patch
FEDPKG_LIBDRM_DIRECTORY=~/fedpkg/libdrm
LIBDRM_VERSION="2.4.117"
FEDORA_DIST_VERSION="-1.fc39"

echo "
Capturing sudo caching first to enable uninterrupted script execution
Only dnf reinstall will be executed with sudo ..."
sudo whoami > /dev/null

git diff libdrm-${LIBDRM_VERSION} hack/assume-single-static-gpu-device > ${PATCH_FILE}

cp ${PATCH_FILE} ${FEDPKG_LIBDRM_DIRECTORY}

# For setting up a build environment for the Fedora package, see
# https://blog.aloni.org/posts/how-to-easily-patch-fedora-packages/
pushd ${FEDPKG_LIBDRM_DIRECTORY}

(
patch --forward <<EOF
diff --git a/libdrm.spec b/libdrm.spec
index 29f6632..dea5baf 100644
--- a/libdrm.spec
+++ b/libdrm.spec
@@ -88,6 +88,8 @@ BuildRequires:  chrpath
 Patch1001:      libdrm-make-dri-perms-okay.patch
 # remove backwards compat not needed on Fedora
 Patch1002:      libdrm-2.4.0-no-bc.patch
+# accelerate rootless X11
+Patch1003:      ${PATCH_FILE}
 
 %description
 Direct Rendering Manager runtime library
EOF
) || true

fedpkg local

sudo dnf reinstall --assumeyes x86_64/libdrm-${LIBDRM_VERSION}${FEDORA_DIST_VERSION}.x86_64.rpm
