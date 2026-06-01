"""Test MCP Bridge TCP connection and all supported methods.

Usage:
    1. Start LibreCAD with MCP Bridge plugin (Plugins -> Start MCP Bridge)
    2. python -m pytest tests/test_mcp_bridge.py -v
"""

import socket
import json
import pytest

HOST = "localhost"
PORT = 12346


def send_command(method: str, params: dict = None) -> dict:
    """Send a JSON command to MCP Bridge and return the response."""
    payload = {"method": method}
    if params:
        payload["params"] = params
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(5)
        s.connect((HOST, PORT))
        s.sendall(json.dumps(payload, ensure_ascii=False).encode("utf-8"))
        # Wait for response (queue-based processing adds ~50ms delay)
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


# --- connection ---

class TestConnection:
    def test_basic_connection(self):
        resp = send_command("getLayers")
        assert resp["status"] == "ok"

    def test_unknown_method(self):
        resp = send_command("nonexistentMethod123")
        assert resp["status"] == "error"
        assert "Unknown method" in resp["message"]


# --- layers ---

class TestLayers:
    def test_set_layer(self):
        resp = send_command("setLayer", {"name": "test_layer"})
        assert resp["status"] == "ok"

    def test_get_layers(self):
        send_command("setLayer", {"name": "layer_for_list_test"})
        resp = send_command("getLayers")
        assert resp["status"] == "ok"
        assert isinstance(resp["layers"], list)

    def test_get_current_layer(self):
        resp = send_command("getCurrentLayer")
        assert resp["status"] == "ok"
        assert "layer" in resp

    def test_delete_layer(self):
        send_command("setLayer", {"name": "layer_to_delete"})
        resp = send_command("deleteLayer", {"name": "layer_to_delete"})
        assert resp["status"] == "ok"


# --- primitives ---

class TestPrimitives:
    def test_add_line(self):
        resp = send_command("addLine", {"x1": 0, "y1": 0, "x2": 10, "y2": 10})
        assert resp["status"] == "ok"

    def test_add_circle(self):
        resp = send_command("addCircle", {"x": 5, "y": 5, "r": 3})
        assert resp["status"] == "ok"

    def test_add_arc(self):
        resp = send_command("addArc", {"x": 0, "y": 0, "r": 5, "a1": 0, "a2": 90})
        assert resp["status"] == "ok"

    def test_add_ellipse(self):
        resp = send_command("addEllipse", {"cx": 5, "cy": 5, "ex": 10, "ey": 5, "ratio": 0.5})
        assert resp["status"] == "ok"

    def test_add_rectangle(self):
        resp = send_command("addRectangle", {"x": 0, "y": 0, "w": 10, "h": 5})
        assert resp["status"] == "ok"

    def test_add_polyline(self):
        resp = send_command("addPolyline", {
            "points": [{"x": 0, "y": 0}, {"x": 5, "y": 5}, {"x": 10, "y": 0}],
            "closed": True
        })
        assert resp["status"] == "ok"

    def test_add_text(self):
        resp = send_command("addText", {"text": "Hello", "x": 0, "y": 0, "size": 5})
        assert resp["status"] == "ok"


# --- architectural ---

class TestArchitectural:
    def test_add_wall(self):
        resp = send_command("addWall", {"x1": 0, "y1": 0, "x2": 10, "y2": 0, "thickness": 0.25})
        assert resp["status"] == "ok"

    def test_add_door(self):
        resp = send_command("addOpening", {"x1": 3, "y1": 0, "x2": 5, "y2": 0, "type": "door"})
        assert resp["status"] == "ok"

    def test_add_window(self):
        resp = send_command("addOpening", {"x1": 6, "y1": 0, "x2": 8, "y2": 0, "type": "window"})
        assert resp["status"] == "ok"


# --- tactical ---

class TestTactical:
    def test_add_friendly_infantry(self):
        resp = send_command("addTacticalSymbol", {"x": 5, "y": 5, "identity": "friendly", "unit": "infantry"})
        assert resp["status"] == "ok"

    def test_add_hostile_armor(self):
        resp = send_command("addTacticalSymbol", {"x": 20, "y": 20, "identity": "hostile", "unit": "armor"})
        assert resp["status"] == "ok"

    def test_add_tactical_line(self):
        resp = send_command("addTacticalLine", {
            "points": [{"x": 0, "y": 0}, {"x": 10, "y": 10}],
            "type": "boundary"
        })
        assert resp["status"] == "ok"


# --- blocks ---

class TestBlocks:
    def test_get_blocks(self):
        resp = send_command("getBlocks")
        assert resp["status"] == "ok"
        assert isinstance(resp["blocks"], list)


# --- encoding ---

class TestEncoding:
    """Verify UTF-8 round-trip for Cyrillic and special characters."""

    def test_cyrillic_layer_name(self):
        resp = send_command("setLayer", {"name": "Окоп_Профиль"})
        assert resp["status"] == "ok"
        resp = send_command("deleteLayer", {"name": "Окоп_Профиль"})
        assert resp["status"] == "ok"

    def test_cyrillic_text(self):
        resp = send_command("addText", {"text": "Привет мир", "x": 0, "y": 0, "size": 5})
        assert resp["status"] == "ok"

    def test_mixed_cyrillic_latin_text(self):
        resp = send_command("addText", {"text": "MCP Тест Test-2025", "x": 0, "y": 0, "size": 5})
        assert resp["status"] == "ok"

    def test_unicode_symbols_text(self):
        resp = send_command("addText", {"text": "→ ← ↑ ↓ ° ±", "x": 0, "y": 0, "size": 5})
        assert resp["status"] == "ok"

    def test_long_cyrillic_text(self):
        text = "Профиль стрелкового окопа"
        resp = send_command("addText", {"text": text, "x": 0, "y": 0, "size": 3})
        assert resp["status"] == "ok"

    def test_cyrillic_in_layers_list(self):
        send_command("setLayer", {"name": "Слой_Тест"})
        resp = send_command("getLayers")
        assert resp["status"] == "ok"
        assert "Слой_Тест" in resp["layers"]
        send_command("deleteLayer", {"name": "Слой_Тест"})

    def test_special_chars_in_text(self):
        resp = send_command("addText", {"text": "M=500кг h=1.1м α=45°", "x": 0, "y": 0, "size": 5})
        assert resp["status"] == "ok"

    def test_json_escaping(self):
        resp = send_command("addText", {"text": 'Quote: "test" and \\backslash', "x": 0, "y": 0, "size": 5})
        assert resp["status"] == "ok"

    def test_empty_text(self):
        resp = send_command("addText", {"text": "", "x": 0, "y": 0, "size": 5})
        assert resp["status"] == "ok"

    def test_large_multibyte_payload(self):
        text = "АБВГДЕЖЗИКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ" * 10
        resp = send_command("addText", {"text": text, "x": 0, "y": 0, "size": 2})
        assert resp["status"] == "ok"


# --- construction: line tools ---

class TestConstructionLines:
    def test_line_2points(self):
        resp = send_command("line2Points", {"x1": 0, "y1": 0, "x2": 5, "y2": 5})
        assert resp["status"] == "ok"

    def test_line_angle(self):
        resp = send_command("lineAngle", {"x": 0, "y": 0, "angle": 45, "length": 10})
        assert resp["status"] == "ok"

    def test_line_angle_default_length(self):
        resp = send_command("lineAngle", {"x": 5, "y": 5, "angle": 90})
        assert resp["status"] == "ok"

    def test_line_horizontal(self):
        resp = send_command("lineHorizontal", {"x": 0, "y": 0, "length": 15})
        assert resp["status"] == "ok"

    def test_line_vertical(self):
        resp = send_command("lineVertical", {"x": 0, "y": 0, "length": 20})
        assert resp["status"] == "ok"

    def test_line_parallel_through_point(self):
        resp = send_command("lineParallelThroughPoint", {
            "lx1": 0, "ly1": 0, "lx2": 10, "ly2": 0, "px": 0, "py": 5
        })
        assert resp["status"] == "ok"

    def test_line_parallel(self):
        resp = send_command("lineParallel", {
            "lx1": 0, "ly1": 0, "lx2": 10, "ly2": 0, "distance": 3
        })
        assert resp["status"] == "ok"

    def test_line_bisector(self):
        resp = send_command("lineBisector", {
            "vx": 0, "vy": 0, "ax": 10, "ay": 0, "bx": 0, "by": 10, "length": 8
        })
        assert resp["status"] == "ok"

    def test_line_tangent_pc(self):
        resp = send_command("lineTangentPC", {"px": 15, "py": 5, "cx": 5, "cy": 5, "r": 3})
        assert resp["status"] == "ok"
        assert "tangentPoints" in resp

    def test_line_tangent_pc_inside_circle(self):
        resp = send_command("lineTangentPC", {"px": 5, "py": 5, "cx": 5, "cy": 5, "r": 3})
        assert resp["status"] == "error"

    def test_line_tangent_cc(self):
        resp = send_command("lineTangentCC", {
            "cx1": 0, "cy1": 0, "r1": 2, "cx2": 15, "cy2": 0, "r2": 2
        })
        assert resp["status"] == "ok"

    def test_line_tangent_orthogonal(self):
        resp = send_command("lineTangentOrthogonal", {
            "cx": 5, "cy": 5, "r": 3,
            "lx1": 0, "ly1": 0, "lx2": 10, "ly2": 0, "length": 10
        })
        assert resp["status"] == "ok"

    def test_line_orthogonal(self):
        resp = send_command("lineOrthogonal", {
            "px": 5, "py": 10, "lx1": 0, "ly1": 0, "lx2": 10, "ly2": 0
        })
        assert resp["status"] == "ok"
        assert "footX" in resp
        assert "footY" in resp

    def test_line_relative_angle(self):
        resp = send_command("lineRelativeAngle", {
            "lx1": 0, "ly1": 0, "lx2": 10, "ly2": 0,
            "px": 5, "py": 5, "angle": -90, "length": 8
        })
        assert resp["status"] == "ok"

    def test_line_snake(self):
        resp = send_command("lineSnake", {
            "points": [{"x": 0, "y": 0}, {"x": 5, "y": 5}, {"x": 10, "y": 0}, {"x": 15, "y": 5}]
        })
        assert resp["status"] == "ok"

    def test_line_snake_x(self):
        resp = send_command("lineSnakeX", {"x": 0, "y": 0, "width": 10, "segments": 3, "gap": 3})
        assert resp["status"] == "ok"

    def test_line_snake_y(self):
        resp = send_command("lineSnakeY", {"x": 0, "y": 0, "height": 10, "segments": 3, "gap": 3})
        assert resp["status"] == "ok"

    def test_line_angle_from_line(self):
        resp = send_command("lineAngleFromLine", {
            "lx1": 0, "ly1": 0, "lx2": 10, "ly2": 0,
            "angle": 45, "length": 8, "t": 0.5
        })
        assert resp["status"] == "ok"

    def test_line_orthogonal_from_line(self):
        resp = send_command("lineOrthogonalFromLine", {
            "lx1": 0, "ly1": 0, "lx2": 10, "ly2": 0,
            "length": 5, "t": 0.5
        })
        assert resp["status"] == "ok"

    def test_line_from_point_to_line(self):
        resp = send_command("lineFromPointToLine", {
            "px": 5, "py": 10, "lx1": 0, "ly1": 0, "lx2": 10, "ly2": 0
        })
        assert resp["status"] == "ok"
        assert "footX" in resp
        assert "footY" in resp


# --- construction: slice/divide ---

class TestSliceDivide:
    def test_slice_divide_line(self):
        resp = send_command("sliceDivideLine", {
            "x1": 0, "y1": 0, "x2": 10, "y2": 0, "segments": 4
        })
        assert resp["status"] == "ok"
        assert "points" in resp
        assert len(resp["points"]) == 5

    def test_slice_divide_line_default(self):
        resp = send_command("sliceDivideLine", {"x1": 0, "y1": 0, "x2": 10, "y2": 0})
        assert resp["status"] == "ok"

    def test_slice_divide_circle(self):
        resp = send_command("sliceDivideCircle", {
            "cx": 5, "cy": 5, "r": 3, "segments": 6
        })
        assert resp["status"] == "ok"
        assert "points" in resp
        assert len(resp["points"]) == 6

    def test_slice_divide_circle_default(self):
        resp = send_command("sliceDivideCircle", {"cx": 0, "cy": 0, "r": 5})
        assert resp["status"] == "ok"


# --- construction: center marks ---

class TestCenterMarks:
    def test_center_mark(self):
        resp = send_command("centerMark", {"cx": 5, "cy": 5, "size": 1.0})
        assert resp["status"] == "ok"

    def test_center_mark_default_size(self):
        resp = send_command("centerMark", {"cx": 10, "cy": 10})
        assert resp["status"] == "ok"

    def test_centerline(self):
        resp = send_command("centerline", {"cx": 5, "cy": 5, "r": 3, "extension": 2})
        assert resp["status"] == "ok"

    def test_centerline_default(self):
        resp = send_command("centerline", {"cx": 0, "cy": 0, "r": 5})
        assert resp["status"] == "ok"
