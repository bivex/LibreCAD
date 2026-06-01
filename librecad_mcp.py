from fastmcp import FastMCP
import socket
import json
from typing import List, Dict, Any, Optional

mcp = FastMCP("LibreCAD_Pro")

LIBRECAD_HOST = "localhost"
LIBRECAD_PORT = 12346


def query_librecad(method: str, params: Dict[str, Any] = None) -> Dict[str, Any]:
    """Send a JSON command to LibreCAD MCP Bridge and return the response."""
    payload = {"method": method}
    if params:
        payload["params"] = params
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(5)
            s.connect((LIBRECAD_HOST, LIBRECAD_PORT))
            s.sendall(json.dumps(payload, ensure_ascii=False).encode("utf-8"))
            data = b""
            while True:
                try:
                    chunk = s.recv(8192)
                    if not chunk:
                        break
                    data += chunk
                    if b"}" in data:
                        break
                except socket.timeout:
                    break
            if not data:
                return {"status": "error", "message": "Empty response"}
            return json.loads(data.decode("utf-8"))
    except Exception as e:
        return {"status": "error", "message": str(e)}


# --- Primitives ---


@mcp.tool()
def add_line(x1: float, y1: float, x2: float, y2: float) -> str:
    """Draw a line from (x1,y1) to (x2,y2)."""
    return str(query_librecad("addLine", {"x1": x1, "y1": y1, "x2": x2, "y2": y2}))


@mcp.tool()
def add_circle(x: float, y: float, r: float) -> str:
    """Draw a circle at center (x,y) with radius r."""
    return str(query_librecad("addCircle", {"x": x, "y": y, "r": r}))


@mcp.tool()
def add_arc(x: float, y: float, r: float, a1: float, a2: float) -> str:
    """Draw an arc at center (x,y), radius r, from angle a1 to a2 (degrees)."""
    return str(query_librecad("addArc", {"x": x, "y": y, "r": r, "a1": a1, "a2": a2}))


@mcp.tool()
def add_ellipse(cx: float, cy: float, ex: float, ey: float, ratio: float = 0.5) -> str:
    """Draw an ellipse with center (cx,cy), major axis endpoint (ex,ey), and ratio."""
    return str(query_librecad("addEllipse", {"cx": cx, "cy": cy, "ex": ex, "ey": ey, "ratio": ratio}))


@mcp.tool()
def add_rectangle(x: float, y: float, w: float, h: float) -> str:
    """Draw a rectangle at (x,y) with width w and height h."""
    return str(query_librecad("addRectangle", {"x": x, "y": y, "w": w, "h": h}))


@mcp.tool()
def add_polyline(points: List[Dict[str, float]], closed: bool = False) -> str:
    """Draw a polyline through a list of {x,y} points. Set closed=True to close the shape."""
    return str(query_librecad("addPolyline", {"points": points, "closed": closed}))


@mcp.tool()
def add_text(text: str, x: float, y: float, size: float = 10.0) -> str:
    """Place text at position (x,y) with given font size."""
    return str(query_librecad("addText", {"text": text, "x": x, "y": y, "size": size}))


@mcp.tool()
def add_point(x: float, y: float) -> str:
    """Draw a point at (x,y)."""
    return str(query_librecad("addPoint", {"x": x, "y": y}))


@mcp.tool()
def add_spline_points(points: List[Dict[str, float]], closed: bool = False) -> str:
    """Draw a spline curve through a list of {x,y} interpolation points. Set closed=True to close."""
    return str(query_librecad("addSplinePoints", {"points": points, "closed": closed}))


# --- Block / Insert Tools ---


@mcp.tool()
def add_insert(name: str, x: float, y: float, sx: float = 1.0, sy: float = 1.0, rotation: float = 0.0) -> str:
    """Insert a block at position (x,y) with scale (sx,sy) and rotation (degrees)."""
    return str(query_librecad("addInsert", {"name": name, "x": x, "y": y, "sx": sx, "sy": sy, "rotation": rotation}))


@mcp.tool()
def add_block_from_disk(path: str) -> str:
    """Load a block definition from a DXF file on disk and return its name."""
    return str(query_librecad("addBlockFromDisk", {"path": path}))


# --- Dimension Tools ---


@mcp.tool()
def add_dim_linear(x1: float, y1: float, x2: float, y2: float, dim_line_offset: float = 10.0, angle: float = 0.0, text: str = "") -> str:
    """Add a linear dimension between two points with optional rotation angle and text override."""
    return str(query_librecad("addDimLinear", {
        "x1": x1, "y1": y1, "x2": x2, "y2": y2,
        "dimLineOffset": dim_line_offset, "angle": angle, "text": text
    }))


@mcp.tool()
def add_dim_aligned(x1: float, y1: float, x2: float, y2: float, dim_line_offset: float = 10.0, text: str = "") -> str:
    """Add an aligned dimension between two points (parallel to the measured line)."""
    return str(query_librecad("addDimAligned", {
        "x1": x1, "y1": y1, "x2": x2, "y2": y2,
        "dimLineOffset": dim_line_offset, "text": text
    }))


@mcp.tool()
def add_dim_radial(cx: float, cy: float, ex: float, ey: float, text: str = "") -> str:
    """Add a radial dimension from center (cx,cy) to point on circle (ex,ey)."""
    return str(query_librecad("addDimRadial", {
        "cx": cx, "cy": cy, "ex": ex, "ey": ey, "text": text
    }))


@mcp.tool()
def add_dim_diametric(cx: float, cy: float, ex: float, ey: float, text: str = "") -> str:
    """Add a diametric dimension from center (cx,cy) to point on circle (ex,ey)."""
    return str(query_librecad("addDimDiametric", {
        "cx": cx, "cy": cy, "ex": ex, "ey": ey, "text": text
    }))


@mcp.tool()
def add_dim_angular(x1: float, y1: float, x2: float, y2: float, vx: float, vy: float, text: str = "") -> str:
    """Add an angular dimension. (x1,y1)-(x2,y2) define two rays from vertex (vx,vy)."""
    return str(query_librecad("addDimAngular", {
        "x1": x1, "y1": y1, "x2": x2, "y2": y2, "vx": vx, "vy": vy, "text": text
    }))


@mcp.tool()
def add_dim_leader(x1: float, y1: float, x2: float, y2: float, text: str = "") -> str:
    """Add a leader dimension (arrow with optional text) from (x1,y1) to (x2,y2)."""
    return str(query_librecad("addDimLeader", {
        "x1": x1, "y1": y1, "x2": x2, "y2": y2, "text": text
    }))


# --- Entity Query Tools ---


@mcp.tool()
def get_all_entities() -> str:
    """Get a list of all entities in the current drawing with their properties (id, type, layer, coordinates)."""
    return str(query_librecad("getAllEntities"))


@mcp.tool()
def get_entity_by_id(eid: float) -> str:
    """Get detailed data for a specific entity by its ID."""
    return str(query_librecad("getEntityById", {"eid": eid}))


# --- Entity Operation Tools ---


@mcp.tool()
def remove_entity(eid: float) -> str:
    """Remove/delete an entity from the drawing by its ID."""
    return str(query_librecad("removeEntity", {"eid": eid}))


@mcp.tool()
def move_entity(eid: float, dx: float, dy: float) -> str:
    """Move an entity by offset (dx, dy)."""
    return str(query_librecad("moveEntity", {"eid": eid, "dx": dx, "dy": dy}))


@mcp.tool()
def rotate_entity(eid: float, cx: float, cy: float, angle: float) -> str:
    """Rotate an entity around center (cx,cy) by angle (degrees)."""
    return str(query_librecad("rotateEntity", {"eid": eid, "cx": cx, "cy": cy, "angle": angle}))


@mcp.tool()
def scale_entity(eid: float, cx: float, cy: float, sx: float, sy: float) -> str:
    """Scale an entity relative to base point (cx,cy) by factors (sx, sy)."""
    return str(query_librecad("scaleEntity", {"eid": eid, "cx": cx, "cy": cy, "sx": sx, "sy": sy}))


# --- Architectural (Interior) Tools ---


@mcp.tool()
def add_wall(x1: float, y1: float, x2: float, y2: float, thickness: float = 0.2) -> str:
    """Draw a wall with specific thickness between two points."""
    return str(query_librecad("addWall", {"x1": x1, "y1": y1, "x2": x2, "y2": y2, "thickness": thickness}))


@mcp.tool()
def add_door(x1: float, y1: float, x2: float, y2: float) -> str:
    """Add a door opening with a swing arc between two points."""
    return str(query_librecad("addOpening", {"x1": x1, "y1": y1, "x2": x2, "y2": y2, "type": "door"}))


@mcp.tool()
def add_window(x1: float, y1: float, x2: float, y2: float) -> str:
    """Add a window opening between two points."""
    return str(query_librecad("addOpening", {"x1": x1, "y1": y1, "x2": x2, "y2": y2, "type": "window"}))


# --- Tactical (Military) Tools ---


@mcp.tool()
def add_unit_symbol(x: float, y: float, identity: str = "friendly", unit: str = "infantry", size: float = 5.0) -> str:
    """Draw a NATO APP-6 tactical symbol.
    identity: friendly (blue box), hostile (red diamond), neutral (green square).
    unit: infantry, armor, artillery, HQ.
    """
    return str(query_librecad("addTacticalSymbol", {"x": x, "y": y, "identity": identity, "unit": unit, "size": size}))


@mcp.tool()
def add_tactical_line(points: List[Dict[str, float]], type: str = "boundary") -> str:
    """Draw tactical lines (Boundaries, Axis of Advance, etc.).
    type: boundary, axis_of_advance, main_attack.
    """
    return str(query_librecad("addTacticalLine", {"points": points, "type": type}))


# --- Layer Tools ---


@mcp.tool()
def set_layer(name: str) -> str:
    """Set the current active layer by name. Creates it if it doesn't exist."""
    return str(query_librecad("setLayer", {"name": name}))


@mcp.tool()
def get_layers() -> str:
    """Get a list of all layers in the current drawing."""
    return str(query_librecad("getLayers"))


@mcp.tool()
def get_current_layer() -> str:
    """Get the name of the currently active layer."""
    return str(query_librecad("getCurrentLayer"))


@mcp.tool()
def delete_layer(name: str) -> str:
    """Delete a layer by name."""
    return str(query_librecad("deleteLayer", {"name": name}))


@mcp.tool()
def get_layer_properties(layer: str) -> str:
    """Get color, line width, and line type for a specific layer."""
    return str(query_librecad("getLayerProperties", {"layer": layer}))


@mcp.tool()
def set_layer_properties(layer: str, color: int = 0, line_width: str = "0.00mm", line_type: str = "SolidLine") -> str:
    """Set color, line width, and line type for a specific layer.
    color: 24-bit RGB integer (e.g. 16711680 for red).
    line_width: string like '0.25mm', '0.50mm'.
    line_type: SolidLine, DashLine, DotLine, CenterLine, DashDotLine, etc.
    """
    return str(query_librecad("setLayerProperties", {
        "layer": layer, "color": color, "lineWidth": line_width, "lineType": line_type
    }))


# --- Block Tools ---


@mcp.tool()
def get_blocks() -> str:
    """Get a list of all blocks in the current drawing."""
    return str(query_librecad("getBlocks"))


# --- Variable Tools ---


@mcp.tool()
def get_variable(key: str) -> str:
    """Get a drawing variable value by key (e.g. DIMTXT, DIMASZ, LUNITS)."""
    return str(query_librecad("getVariable", {"key": key}))


@mcp.tool()
def set_variable(key: str, value: float, code: int = 70) -> str:
    """Set a drawing variable. code: 70 for integers, 40 for doubles."""
    return str(query_librecad("setVariable", {"key": key, "value": value, "code": code}))


# --- Construction Line Tools ---


@mcp.tool()
def line_2points(x1: float, y1: float, x2: float, y2: float) -> str:
    """Draw a line between two points."""
    return str(query_librecad("line2Points", {"x1": x1, "y1": y1, "x2": x2, "y2": y2}))


@mcp.tool()
def line_angle(x: float, y: float, angle: float, length: float = 10.0) -> str:
    """Draw a line from point (x,y) at given angle (degrees) with specified length."""
    return str(query_librecad("lineAngle", {"x": x, "y": y, "angle": angle, "length": length}))


@mcp.tool()
def line_horizontal(x: float, y: float, length: float = 10.0) -> str:
    """Draw a horizontal line from (x,y) with given length."""
    return str(query_librecad("lineHorizontal", {"x": x, "y": y, "length": length}))


@mcp.tool()
def line_vertical(x: float, y: float, length: float = 10.0) -> str:
    """Draw a vertical line from (x,y) with given length."""
    return str(query_librecad("lineVertical", {"x": x, "y": y, "length": length}))


@mcp.tool()
def line_parallel_through_point(
    lx1: float, ly1: float, lx2: float, ly2: float, px: float, py: float
) -> str:
    """Draw a line parallel to reference line through a given point.
    Reference line: (lx1,ly1)-(lx2,ly2), passes through (px,py)."""
    return str(query_librecad("lineParallelThroughPoint", {
        "lx1": lx1, "ly1": ly1, "lx2": lx2, "ly2": ly2, "px": px, "py": py
    }))


@mcp.tool()
def line_parallel(
    lx1: float, ly1: float, lx2: float, ly2: float, distance: float
) -> str:
    """Draw a line parallel to reference line at given distance.
    Reference line: (lx1,ly1)-(lx2,ly2)."""
    return str(query_librecad("lineParallel", {
        "lx1": lx1, "ly1": ly1, "lx2": lx2, "ly2": ly2, "distance": distance
    }))


@mcp.tool()
def line_bisector(
    vx: float, vy: float, ax: float, ay: float, bx: float, by: float, length: float = 10.0
) -> str:
    """Draw the angle bisector at vertex (vx,vy) between rays to (ax,ay) and (bx,by)."""
    return str(query_librecad("lineBisector", {
        "vx": vx, "vy": vy, "ax": ax, "ay": ay, "bx": bx, "by": by, "length": length
    }))


@mcp.tool()
def line_tangent_pc(px: float, py: float, cx: float, cy: float, r: float) -> str:
    """Draw tangent lines from point (px,py) to circle at (cx,cy) with radius r."""
    return str(query_librecad("lineTangentPC", {"px": px, "py": py, "cx": cx, "cy": cy, "r": r}))


@mcp.tool()
def line_tangent_cc(
    cx1: float, cy1: float, r1: float, cx2: float, cy2: float, r2: float
) -> str:
    """Draw common tangent lines between two circles."""
    return str(query_librecad("lineTangentCC", {
        "cx1": cx1, "cy1": cy1, "r1": r1, "cx2": cx2, "cy2": cy2, "r2": r2
    }))


@mcp.tool()
def line_tangent_orthogonal(
    cx: float, cy: float, r: float,
    lx1: float, ly1: float, lx2: float, ly2: float, length: float = 10.0
) -> str:
    """Draw tangent to circle perpendicular to a reference line."""
    return str(query_librecad("lineTangentOrthogonal", {
        "cx": cx, "cy": cy, "r": r,
        "lx1": lx1, "ly1": ly1, "lx2": lx2, "ly2": ly2, "length": length
    }))


@mcp.tool()
def line_orthogonal(
    px: float, py: float, lx1: float, ly1: float, lx2: float, ly2: float
) -> str:
    """Draw perpendicular line from point (px,py) to line (lx1,ly1)-(lx2,ly2)."""
    return str(query_librecad("lineOrthogonal", {
        "px": px, "py": py, "lx1": lx1, "ly1": ly1, "lx2": lx2, "ly2": ly2
    }))


@mcp.tool()
def line_relative_angle(
    lx1: float, ly1: float, lx2: float, ly2: float,
    px: float, py: float, angle: float, length: float = 10.0
) -> str:
    """Draw a line at relative angle from reference line through point."""
    return str(query_librecad("lineRelativeAngle", {
        "lx1": lx1, "ly1": ly1, "lx2": lx2, "ly2": ly2,
        "px": px, "py": py, "angle": angle, "length": length
    }))


@mcp.tool()
def line_snake(points: List[Dict[str, float]]) -> str:
    """Draw connected line segments through a list of {x,y} points."""
    return str(query_librecad("lineSnake", {"points": points}))


@mcp.tool()
def line_snake_x(
    x: float, y: float, width: float = 10.0, segments: int = 3, gap: float = 3.0
) -> str:
    """Draw a horizontal snake pattern (alternating horizontal segments connected by verticals)."""
    return str(query_librecad("lineSnakeX", {
        "x": x, "y": y, "width": width, "segments": segments, "gap": gap
    }))


@mcp.tool()
def line_snake_y(
    x: float, y: float, height: float = 10.0, segments: int = 3, gap: float = 3.0
) -> str:
    """Draw a vertical snake pattern (alternating vertical segments connected by horizontals)."""
    return str(query_librecad("lineSnakeY", {
        "x": x, "y": y, "height": height, "segments": segments, "gap": gap
    }))


@mcp.tool()
def line_angle_from_line(
    lx1: float, ly1: float, lx2: float, ly2: float,
    angle: float, length: float = 10.0, t: float = 0.5
) -> str:
    """Draw a line at given angle from a reference line, starting at offset t (0=start, 1=end)."""
    return str(query_librecad("lineAngleFromLine", {
        "lx1": lx1, "ly1": ly1, "lx2": lx2, "ly2": ly2,
        "angle": angle, "length": length, "t": t
    }))


@mcp.tool()
def line_orthogonal_from_line(
    lx1: float, ly1: float, lx2: float, ly2: float,
    length: float = 10.0, t: float = 0.5
) -> str:
    """Draw perpendicular from a point on line at offset t (0=start, 1=end)."""
    return str(query_librecad("lineOrthogonalFromLine", {
        "lx1": lx1, "ly1": ly1, "lx2": lx2, "ly2": ly2, "length": length, "t": t
    }))


@mcp.tool()
def line_from_point_to_line(
    px: float, py: float, lx1: float, ly1: float, lx2: float, ly2: float
) -> str:
    """Draw shortest line (perpendicular drop) from point to line."""
    return str(query_librecad("lineFromPointToLine", {
        "px": px, "py": py, "lx1": lx1, "ly1": ly1, "lx2": lx2, "ly2": ly2
    }))


# --- Construction: Slice/Divide Tools ---


@mcp.tool()
def slice_divide_line(
    x1: float, y1: float, x2: float, y2: float, segments: int = 2, mark_size: float = 0.5
) -> str:
    """Divide a line into N equal segments and mark division points."""
    return str(query_librecad("sliceDivideLine", {
        "x1": x1, "y1": y1, "x2": x2, "y2": y2, "segments": segments, "markSize": mark_size
    }))


@mcp.tool()
def slice_divide_circle(
    cx: float, cy: float, r: float, segments: int = 4, mark_size: float = 0.3
) -> str:
    """Divide a circle into N equal arcs and mark division points."""
    return str(query_librecad("sliceDivideCircle", {
        "cx": cx, "cy": cy, "r": r, "segments": segments, "markSize": mark_size
    }))


# --- Construction: Center Mark Tools ---


@mcp.tool()
def center_mark(cx: float, cy: float, size: float = 1.0) -> str:
    """Draw a cross mark at center point (cx,cy)."""
    return str(query_librecad("centerMark", {"cx": cx, "cy": cy, "size": size}))


@mcp.tool()
def centerline(cx: float, cy: float, r: float, extension: float = 2.0) -> str:
    """Draw centerline cross through circle center, extending beyond radius."""
    return str(query_librecad("centerline", {"cx": cx, "cy": cy, "r": r, "extension": extension}))


if __name__ == "__main__":
    mcp.run()
