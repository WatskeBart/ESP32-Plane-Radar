#pragma once

namespace ui {

/** Draw the static sonar/radar grid (black disc, green overlay, labels). */
void radarDisplayDraw();

/** Redraw aircraft only (blits cached grid; no full-screen clear). */
void radarDisplayRefreshAircraft();

/** Force label metrics to be recomputed on next draw (call after changing range presets). */
void radarDisplayInvalidateLabelMetrics();

}  // namespace ui
