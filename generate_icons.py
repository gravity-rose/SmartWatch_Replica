#!/usr/bin/env python3
"""Generate PNG icons and squircle bezel data for the Pebble watchface."""
import cairo
import math
import os

ICON_SIZE = 25
HEART_SIZE = 30
OUT_DIR = "resources/images"
os.makedirs(OUT_DIR, exist_ok=True)

CYAN = (0, 1, 1, 1)

def new_surface(size=ICON_SIZE):
    s = cairo.ImageSurface(cairo.FORMAT_ARGB32, size, size)
    c = cairo.Context(s)
    c.set_source_rgba(0, 0, 0, 0)
    c.paint()
    c.set_source_rgba(*CYAN)
    return s, c

def save(surface, name):
    surface.write_to_png(os.path.join(OUT_DIR, name))
    print(f"  Created {name}")

def draw_heart(ctx, cx, cy, size):
    s = size / 2.0
    ctx.new_path()
    ctx.move_to(cx, cy + s * 0.4)
    ctx.curve_to(cx, cy - s * 0.6, cx - s, cy - s * 0.6, cx - s, cy)
    ctx.curve_to(cx - s, cy + s * 0.5, cx, cy + s, cx, cy + s)
    ctx.curve_to(cx, cy + s, cx + s, cy + s * 0.5, cx + s, cy)
    ctx.curve_to(cx + s, cy - s * 0.6, cx, cy - s * 0.6, cx, cy + s * 0.4)
    ctx.close_path()
    ctx.fill()

# --- Heart icon (30x30, bigger) ---
s, c = new_surface(HEART_SIZE)
draw_heart(c, 15, 13, 14)
save(s, "icon_heart.png")

# --- Steps icon (footprint) ---
s, c = new_surface()
c.set_line_width(1.5)
c.new_path()
c.arc(8, 6, 2, 0, 2 * math.pi)
c.fill()
c.new_path()
c.move_to(6, 9)
c.curve_to(5, 14, 5, 18, 7, 20)
c.line_to(10, 20)
c.curve_to(12, 18, 11, 14, 10, 9)
c.close_path()
c.fill()
c.new_path()
c.arc(17, 9, 2, 0, 2 * math.pi)
c.fill()
c.new_path()
c.move_to(15, 12)
c.curve_to(14, 16, 14, 20, 16, 22)
c.line_to(19, 22)
c.curve_to(21, 20, 20, 16, 19, 12)
c.close_path()
c.fill()
save(s, "icon_steps.png")

# --- Calories/flame icon ---
s, c = new_surface()
c.new_path()
c.move_to(12.5, 2)
c.curve_to(10, 6, 5, 10, 5, 16)
c.curve_to(5, 21, 9, 24, 12.5, 24)
c.curve_to(16, 24, 20, 21, 20, 16)
c.curve_to(20, 10, 15, 6, 12.5, 2)
c.close_path()
c.fill()
c.set_operator(cairo.OPERATOR_CLEAR)
c.new_path()
c.move_to(12.5, 12)
c.curve_to(11, 14, 9, 16, 9, 18)
c.curve_to(9, 21, 11, 23, 12.5, 23)
c.curve_to(14, 23, 16, 21, 16, 18)
c.curve_to(16, 16, 14, 14, 12.5, 12)
c.close_path()
c.fill()
save(s, "icon_calories.png")

# --- Weather: Sunny ---
s, c = new_surface()
cx, cy = 12.5, 12.5
c.arc(cx, cy, 5, 0, 2 * math.pi)
c.fill()
c.set_line_width(1.5)
for i in range(8):
    angle = i * math.pi / 4
    x1 = cx + 7 * math.cos(angle)
    y1 = cy + 7 * math.sin(angle)
    x2 = cx + 10 * math.cos(angle)
    y2 = cy + 10 * math.sin(angle)
    c.move_to(x1, y1)
    c.line_to(x2, y2)
    c.stroke()
save(s, "weather_sunny.png")

# --- Weather: Cloudy ---
s, c = new_surface()
c.arc(10, 14, 5, 0, 2 * math.pi)
c.fill()
c.arc(16, 12, 5, 0, 2 * math.pi)
c.fill()
c.arc(8, 16, 3, 0, 2 * math.pi)
c.fill()
c.arc(18, 15, 4, 0, 2 * math.pi)
c.fill()
c.rectangle(5, 15, 18, 5)
c.fill()
save(s, "weather_cloudy.png")

# --- Weather: Rainy ---
s, c = new_surface()
c.arc(9, 8, 4, 0, 2 * math.pi)
c.fill()
c.arc(15, 7, 4, 0, 2 * math.pi)
c.fill()
c.arc(7, 10, 2.5, 0, 2 * math.pi)
c.fill()
c.arc(17, 9, 3, 0, 2 * math.pi)
c.fill()
c.rectangle(5, 9, 15, 4)
c.fill()
c.set_line_width(1.5)
for x, y_start in [(8, 16), (12, 15), (16, 16), (10, 19), (14, 19)]:
    c.move_to(x, y_start)
    c.line_to(x - 1, y_start + 3)
    c.stroke()
save(s, "weather_rainy.png")

# --- Weather: Snowy ---
s, c = new_surface()
c.arc(9, 8, 4, 0, 2 * math.pi)
c.fill()
c.arc(15, 7, 4, 0, 2 * math.pi)
c.fill()
c.arc(7, 10, 2.5, 0, 2 * math.pi)
c.fill()
c.arc(17, 9, 3, 0, 2 * math.pi)
c.fill()
c.rectangle(5, 9, 15, 4)
c.fill()
for x, y in [(8, 16), (14, 16), (11, 19), (17, 19), (8, 22), (14, 22)]:
    c.arc(x, y, 1.2, 0, 2 * math.pi)
    c.fill()
save(s, "weather_snowy.png")

# --- Weather: Thunder ---
s, c = new_surface()
c.arc(9, 7, 4, 0, 2 * math.pi)
c.fill()
c.arc(15, 6, 4, 0, 2 * math.pi)
c.fill()
c.arc(7, 9, 2.5, 0, 2 * math.pi)
c.fill()
c.arc(17, 8, 3, 0, 2 * math.pi)
c.fill()
c.rectangle(5, 8, 15, 4)
c.fill()
c.new_path()
c.move_to(14, 13)
c.line_to(10, 18)
c.line_to(13, 18)
c.line_to(10, 24)
c.line_to(16, 17)
c.line_to(13, 17)
c.line_to(14, 13)
c.close_path()
c.fill()
save(s, "weather_thunder.png")

print("All icons generated successfully!\n")

# ============================================================
# Generate squircle bezel tick coordinates
# ============================================================
print("Generating squircle bezel data...")

N_EXP = 5
CX, CY = 100, 114
A, B = 97, 112
TICK_SHORT = 6
TICK_LONG = 10
NUM_TICKS = 60

lines = []
lines.append("// Auto-generated squircle bezel tick coordinates (superellipse n=5)")
lines.append("// Format: {outer_x, outer_y, inner_x, inner_y}")
lines.append(f"#define BEZEL_NUM_TICKS {NUM_TICKS}")
lines.append(f"#define BEZEL_TICK_SHORT {TICK_SHORT}")
lines.append(f"#define BEZEL_TICK_LONG {TICK_LONG}")
lines.append("")
lines.append(f"static const int16_t s_bezel_short[{NUM_TICKS}][4] = {{")

for i in range(NUM_TICKS):
    t = i * 2 * math.pi / NUM_TICKS
    sin_t = math.sin(t)
    cos_t = math.cos(t)
    dx = sin_t
    dy = -cos_t
    adx = max(abs(dx), 1e-10)
    ady = max(abs(dy), 1e-10)
    r = ((adx / A) ** N_EXP + (ady / B) ** N_EXP) ** (-1.0 / N_EXP)
    ox = CX + r * dx
    oy = CY + r * dy
    ix_s = ox - TICK_SHORT * dx
    iy_s = oy - TICK_SHORT * dy
    lines.append(f"  {{{int(round(ox))}, {int(round(oy))}, {int(round(ix_s))}, {int(round(iy_s))}}},")

lines.append("};")
lines.append("")
lines.append(f"static const int16_t s_bezel_long[{NUM_TICKS}][4] = {{")

for i in range(NUM_TICKS):
    t = i * 2 * math.pi / NUM_TICKS
    sin_t = math.sin(t)
    cos_t = math.cos(t)
    dx = sin_t
    dy = -cos_t
    adx = max(abs(dx), 1e-10)
    ady = max(abs(dy), 1e-10)
    r = ((adx / A) ** N_EXP + (ady / B) ** N_EXP) ** (-1.0 / N_EXP)
    ox = CX + r * dx
    oy = CY + r * dy
    ix_l = ox - TICK_LONG * dx
    iy_l = oy - TICK_LONG * dy
    lines.append(f"  {{{int(round(ox))}, {int(round(oy))}, {int(round(ix_l))}, {int(round(iy_l))}}},")

lines.append("};")

header_path = os.path.join("src", "c", "squircle_data.h")
with open(header_path, "w") as f:
    f.write("\n".join(lines) + "\n")
print(f"  Created {header_path}")
print("Done!")
