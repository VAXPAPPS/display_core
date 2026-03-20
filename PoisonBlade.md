# PoisonBlade 🗡️

A lightweight, fast hybrid window manager (tiling + floating), designed for the VAXP OS system.

## Installation

```bash
sudo ./install.sh
```

---

## vaxp Commands

### Window Management (node)

```bash
# Focus a window
vaxp node -f {next,prev,north,south,east,west}
vaxp node -f any.hidden         # any hidden window

# Swap/move the window
vaxp node -s {north,south,east,west}  # swap with neighbor

# Change window state
vaxp node -t tiled              # tiled
vaxp node -t floating           # floating
vaxp node -t fullscreen         # fullscreen
vaxp node -t maximized          # maximized (respects panel)
vaxp node -t ~maximized         # toggle maximized

# Hide/Show
vaxp node -g hidden=on          # hide
vaxp node -g hidden=off         # show
vaxp node any.hidden -g hidden=off  # show any hidden

# Close/Kill
vaxp node -c                    # close window
vaxp node -k                    # kill window
```

### Desktop Management (desktop)

```bash
# Change layout
vaxp desktop -l tiled           # normal tiling
vaxp desktop -l monocle         # single window
vaxp desktop -l tall            # master on the left
vaxp desktop -l rtall           # master on the right
vaxp desktop -l wide            # master on top
vaxp desktop -l rwide           # master on bottom
vaxp desktop -l grid            # two-row grid
vaxp desktop -l rgrid           # two-column grid
vaxp desktop -l even            # even distribution

# Switch between desktops
vaxp desktop -f next            # next
vaxp desktop -f prev            # previous
vaxp desktop -f ^1              # desktop 1
vaxp desktop -f ^2              # desktop 2
vaxp desktop -f last            # last desktop

# Create a new desktop
vaxp monitor -a "Desktop1"      # add a desktop

# Remove current desktop
vaxp desktop -r                 # remove current desktop

# Rename
vaxp desktop -n "WorkSpace"     # name the desktop
```

### Move Windows Between Desktops

```bash
# Move current window to a specific desktop
vaxp node -d ^1                 # move to desktop 1
vaxp node -d ^2                 # move to desktop 2
vaxp node -d next               # move to next
vaxp node -d prev               # move to previous

# Move and follow
vaxp node -d ^1 --follow        # move + follow
vaxp node -d next --follow      # move to next + follow

# Move a window by ID
vaxp node 0x12345678 -d ^3      # move specified window to desktop 3
```

### Configuration (config)

```bash
# Floating mode
vaxp config floating_mode true  # enable (+ snap_to_edge enabled automatically)
vaxp config floating_mode false # disable

# snap_to_edge
vaxp config snap_to_edge true   # enable
vaxp config snap_threshold 20   # edge sensitivity (pixels)
vaxp config snap_show_preview true  # show snap preview

# Borders
vaxp config border_width 2
vaxp config focused_border_color "#5e81ac"
vaxp config normal_border_color "#3b4252"

# Gaps
vaxp config window_gap 10
vaxp config top_padding 30      # for top panel
vaxp config bottom_padding 0

# Opacity
vaxp config focus_opacity true
vaxp config inactive_opacity 0.85
vaxp config active_opacity 1.0
```

### Subscribe (subscribe)

```bash
vaxp subscribe all              # all events
vaxp subscribe node             # window events
vaxp subscribe desktop          # desktop events
```

### Query (query)

```bash
vaxp query -N                   # list windows
vaxp query -D                   # list desktops
vaxp query -M                   # list monitors
vaxp query -T                   # window tree
```

### Rules (rule)

```bash
vaxp rule -a "Firefox" desktop=^2 state=floating
vaxp rule -a "Steam" state=floating
vaxp rule -l                    # list rules
vaxp rule -r "Firefox"          # remove rule
```

### Multi-monitor Management (monitor)

```bash
# Switch monitors
vaxp monitor -f next            # next monitor
vaxp monitor -f prev            # previous monitor
vaxp monitor -f primary         # primary monitor

# Move window to another monitor
vaxp node -m next               # move to next monitor
vaxp node -m prev               # move to previous monitor
vaxp node -m ^1                 # move to specified monitor

# Add a desktop to a monitor
vaxp monitor HDMI-1 -a "Work"   # add desktop to specific monitor

# Rename monitor
vaxp monitor -n "Main"          # name current monitor
```

### Preselection (Preselection)

```bash
# Set direction for a new tiled window
vaxp node -p north              # new window will appear above
vaxp node -p south              # new window will appear below
vaxp node -p east               # new window will appear to the right
vaxp node -p west               # new window will appear to the left

# Set size ratio
vaxp node -o 0.3                # new window 30%

# Cancel preselection
vaxp node -p cancel             # cancel preselection
```

### Resize Windows (resize)

```bash
# Resize in pixels
vaxp node -z left -20 0         # shrink from left
vaxp node -z right 20 0         # expand to the right
vaxp node -z top 0 -20          # shrink from top
vaxp node -z bottom 0 20        # expand to bottom

# Change split ratio
vaxp node @parent -r 0.6        # 60% for the current window

# Directional resize
vaxp node -z top_right 20 -20   # expand top-right
vaxp node -z bottom_left -20 20 # expand bottom-left

# Balance sizes
vaxp node @/ -B                 # balance root (all windows)
vaxp node @parent -B            # balance siblings
```

### Other

```bash
vaxp wm -r                      # restart
vaxp wm -d                      # stop
```

---

## Super+Tab Shortcuts

| Shortcut | Function |
|----------|----------|
| Super+Tab | Switcher open + next |
| Super+Shift+Tab | previous |
| Release Super | apply selection |
| Escape | cancel |
| Enter | apply immediately |
