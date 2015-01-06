#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/bitcredit.png
ICON_DST=../../src/qt/res/icons/bitcredit.ico
convert ${ICON_SRC} -resize 16x16 bitcredit-16.png
convert ${ICON_SRC} -resize 32x32 bitcredit-32.png
convert ${ICON_SRC} -resize 48x48 bitcredit-48.png
convert bitcredit-16.png bitcredit-32.png bitcredit-48.png ${ICON_DST}

