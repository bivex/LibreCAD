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


# --- Block Tools ---


@mcp.tool()
def get_blocks() -> str:
    """Get a list of all blocks in the current drawing."""
    return str(query_librecad("getBlocks"))


if __name__ == "__main__":
    mcp.run()
