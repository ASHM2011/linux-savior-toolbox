#!/bin/bash
# Linux Savior Toolbox launcher script
# Ensures proper Qt environment for native dialogs and theming

export QT_QPA_PLATFORMTHEME=gtk2
export QT_STYLE_OVERRIDE=adwaita
export XDG_CURRENT_DESKTOP="${XDG_CURRENT_DESKTOP:-GNOME}"

# Set icon theme
if [ -z "$ICON_THEME" ]; then
    export ICON_THEME=Yaru
fi

# Launch the application
exec /usr/bin/LinuxSaviorToolbox "$@"
