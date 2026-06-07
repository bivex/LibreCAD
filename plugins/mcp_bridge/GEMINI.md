# LibreCAD MCP Bridge - API Reference

This plugin creates a TCP bridge (port 12346) to control LibreCAD via JSON commands. It is exposed to Gemini CLI via the `librecad-mcp` server.

## Available MCP Tools
- `librecad_invoke(method, params)`: Execute any bridge command.
- `librecad_list_methods()`: Get a list of all 72+ supported methods.
- `librecad_get_entities()`: Quick access to all drawing data.

## Key Architectural Commands
### `addWall(x1, y1, x2, y2, thickness)`
Draws a double-line wall.

### `addWindow(x, y, width, depth, sill, angle, layer)`
Draws a detailed window with frame, glass, and sill.

### `calculateArea(points, label, unit, font)`
Calculates area in m2, draws a multi-line MText label in the center. Use `unit="mm"` if drawing in millimeters.

### `joinWalls(wall1_eids, wall2_eids)`
Trims two intersecting walls at a corner. `wall1_eids` is an array `[inner_eid, outer_eid]`.

### `trimLine(eid, side, ref_eid)`
Trims line `eid` to its intersection with `ref_eid`. `side` can be `"start"` or `"end"`.

## Primitives
- `addLine(x1, y1, x2, y2)`
- `addCircle(x, y, r)`
- `addArc(x, y, r, a1, a2)`
- `addEllipse(cx, cy, ex, ey, ratio, a1, a2)`
- `addPolyline(points, closed)`
- `addMText(text, x, y, height, angle, font)` - Use `font="unicode"` for Cyrillic support.
- `addHatch(points, angle, distance)` - Custom geometric hatching algorithm.

## Transformation & Interactivity
- `moveEntity(eid, dx, dy, copy)` - Set `copy=true` to clone.
- `getPoint(message)` - Asks user to click a point in LibreCAD.
- `getEntity(message)` - Asks user to select an entity.

---
**Note**: Always ensure MCP Bridge is running in LibreCAD (Plugins -> Start MCP Bridge) before calling these tools.
