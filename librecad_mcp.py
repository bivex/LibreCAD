from fastmcp import FastMCP
import socket
import json
from typing import List, Dict, Any

mcp = FastMCP("LibreCAD_Pro")

LIBRECAD_HOST = "localhost"
LIBRECAD_PORT = 12346

def query_librecad(method: str, params: Dict[str, Any]) -> Dict[str, Any]:
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((LIBRECAD_HOST, LIBRECAD_PORT))
            s.sendall(json.dumps({"method": method, "params": params}).encode('utf-8'))
            data = s.recv(8192)
            return json.loads(data.decode('utf-8')) if data else {"status": "error"}
    except Exception as e:
        return {"status": "error", "message": str(e)}

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
    """
    Draw a NATO APP-6 tactical symbol.
    identity: friendly (blue box), hostile (red diamond), neutral (green square).
    unit: infantry, armor, artillery, HQ.
    """
    return str(query_librecad("addTacticalSymbol", {"x": x, "y": y, "identity": identity, "unit": unit, "size": size}))

@mcp.tool()
def add_tactical_line(points: List[Dict[str, float]], type: str = "boundary") -> str:
    """
    Draw tactical lines (Boundaries, Axis of Advance, etc.).
    type: boundary, axis_of_advance, main_attack.
    """
    return str(query_librecad("addTacticalLine", {"points": points, "type": type}))

@mcp.tool()
def set_layer(name: str) -> str:
    return str(query_librecad("setLayer", {"name": name}))

if __name__ == "__main__":
    mcp.run()
