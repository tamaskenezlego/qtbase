# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_commandline_option(accessibility TYPE boolean)
qt_commandline_option(direct2d TYPE boolean)
qt_commandline_option(directfb TYPE boolean)
qt_commandline_option(directwrite TYPE boolean)
qt_commandline_option(egl TYPE boolean)
qt_commandline_option(eglfs TYPE boolean)
qt_commandline_option(evdev TYPE boolean)
qt_commandline_option(fontconfig TYPE boolean)
qt_commandline_option(freetype TYPE enum VALUES no qt system)
qt_commandline_option(emojisegmenter TYPE boolean)
qt_commandline_option(gbm TYPE boolean)
qt_commandline_option(gif TYPE boolean)
qt_commandline_option(harfbuzz TYPE enum VALUES no qt system)
qt_commandline_option(ico TYPE boolean)
qt_commandline_option(imf TYPE boolean NAME qqnx_imf)
qt_commandline_option(kms TYPE boolean)
qt_commandline_option(lgmon TYPE boolean)
qt_commandline_option(libinput TYPE boolean)
qt_commandline_option(libjpeg TYPE enum VALUES no qt system)
qt_commandline_option(libmd4c TYPE enum VALUES no qt system)
qt_commandline_option(libpng TYPE enum VALUES no qt system)
qt_commandline_option(linuxfb TYPE boolean)
qt_commandline_option(mtdev TYPE boolean)
qt_commandline_option(opengl TYPE optionalString VALUES no yes desktop es2 dynamic)
qt_commandline_option(opengl-es-2 TYPE void NAME opengl VALUE es2)
qt_commandline_option(opengles3 TYPE boolean)
qt_commandline_option(openvg TYPE boolean)
qt_commandline_option(qpa TYPE string NAME qpa_platforms)
qt_commandline_option(default-qpa TYPE string NAME qpa_default_platform)
qt_commandline_option(sm TYPE boolean NAME sessionmanager)
qt_commandline_option(tslib TYPE boolean)
qt_commandline_option(vulkan TYPE boolean)
qt_commandline_option(xcb TYPE boolean)
qt_commandline_option(bundled-xcb-xinput TYPE boolean)
qt_commandline_option(xcb-native-painting TYPE boolean)
qt_commandline_option(xcb-xlib TYPE boolean)
qt_commandline_option(xkbcommon TYPE boolean)
