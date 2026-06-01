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
        s.sendall(json.dumps(payload).encode("utf-8"))
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
