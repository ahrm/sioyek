#!/usr/bin/env bash
#This script allows sioyek to work correctly on non-US keyboards

mkdir ~/.config/sioyek
echo "use_legacy_keybinds 0" > ~/.config/sioyek/prefs_user.config
cp ./pdf_viewer/keys_new.config ~/.config/sioyek/keys_user.config

