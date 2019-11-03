#!/usr/bin/env bash

set -e -x

# KDE Frameworks
brew tap kde-mac/kde
"$(brew --repo kde-mac/kde)/tools/do-caveats.sh"

brew install qt5 \
    kde-mac/kde/kf5-knotifications \
    kde-mac/kde/kf5-extra-cmake-modules
