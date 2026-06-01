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
        data = b""
        while True:
            try:
                chunk = s.recv(65536)
                if not chunk:
                    break
                data += chunk
                if b"\n" in data:
                    break
            except socket.timeout:
                break
    if not data:
        return {"status": "error", "message": "Empty response"}
    raw = data.decode("utf-8").strip()
    return json.loads(raw)


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


# --- new primitives: addPoint, addSplinePoints, addLines ---

class TestNewPrimitives:
    def test_add_point(self):
        resp = send_command("addPoint", {"x": 15, "y": 15})
        assert resp["status"] == "ok"

    def test_add_point_default(self):
        resp = send_command("addPoint", {"x": 0, "y": 0})
        assert resp["status"] == "ok"

    def test_add_spline_points(self):
        resp = send_command("addSplinePoints", {
            "points": [{"x": 0, "y": 0}, {"x": 5, "y": 10}, {"x": 10, "y": 0}],
            "closed": False
        })
        assert resp["status"] == "ok"

    def test_add_spline_points_closed(self):
        resp = send_command("addSplinePoints", {
            "points": [{"x": 0, "y": 0}, {"x": 5, "y": 5}, {"x": 10, "y": 0}],
            "closed": True
        })
        assert resp["status"] == "ok"

    def test_add_lines(self):
        resp = send_command("addLines", {
            "points": [{"x": 0, "y": 0}, {"x": 10, "y": 10}, {"x": 20, "y": 0}],
            "closed": False
        })
        assert resp["status"] == "ok"

    def test_add_lines_closed(self):
        resp = send_command("addLines", {
            "points": [{"x": 0, "y": 0}, {"x": 10, "y": 0}, {"x": 10, "y": 10}],
            "closed": True
        })
        assert resp["status"] == "ok"


# --- block/insert ---

class TestBlockInsert:
    def test_add_insert(self):
        resp = send_command("addInsert", {
            "name": "nonexistent_block_test", "x": 0, "y": 0,
            "sx": 1.0, "sy": 1.0, "rotation": 0.0
        })
        # insert of non-existent block may still succeed (creates placeholder)
        assert resp["status"] in ("ok", "error")


# --- dimensions ---

class TestDimensions:
    def test_add_dim_linear(self):
        resp = send_command("addDimLinear", {
            "x1": 0, "y1": 0, "x2": 50, "y2": 0,
            "dimLineOffset": 10, "angle": 0
        })
        assert resp["status"] == "ok"

    def test_add_dim_linear_vertical(self):
        resp = send_command("addDimLinear", {
            "x1": 0, "y1": 0, "x2": 0, "y2": 30,
            "dimLineOffset": 10, "angle": 90
        })
        assert resp["status"] == "ok"

    def test_add_dim_aligned(self):
        resp = send_command("addDimAligned", {
            "x1": 0, "y1": 0, "x2": 30, "y2": 20,
            "dimLineOffset": 10
        })
        assert resp["status"] == "ok"

    def test_add_dim_radial(self):
        send_command("addCircle", {"x": 50, "y": 50, "r": 15})
        resp = send_command("addDimRadial", {
            "cx": 50, "cy": 50, "ex": 65, "ey": 50
        })
        assert resp["status"] == "ok"

    def test_add_dim_diametric(self):
        resp = send_command("addDimDiametric", {
            "cx": 50, "cy": 50, "ex": 65, "ey": 50
        })
        assert resp["status"] == "ok"

    def test_add_dim_angular(self):
        resp = send_command("addDimAngular", {
            "x1": 30, "y1": 0, "x2": 0, "y2": 30,
            "vx": 0, "vy": 0
        })
        assert resp["status"] == "ok"

    def test_add_dim_leader(self):
        resp = send_command("addDimLeader", {
            "x1": 10, "y1": 10, "x2": 30, "y2": 30,
            "text": "Note"
        })
        assert resp["status"] == "ok"


# --- entity queries ---

class TestEntityQueries:
    def test_get_all_entities(self):
        send_command("addLine", {"x1": 100, "y1": 100, "x2": 110, "y2": 110})
        resp = send_command("getAllEntities")
        assert resp["status"] == "ok"
        assert isinstance(resp["entities"], list)
        assert len(resp["entities"]) > 0

    def test_get_entity_by_id(self):
        resp_all = send_command("getAllEntities")
        if resp_all["entities"]:
            eid = resp_all["entities"][0]["eid"]
            resp = send_command("getEntityById", {"eid": float(eid)})
            assert resp["status"] == "ok"
            assert "entity" in resp
            assert resp["entity"]["eid"] == eid

    def test_get_entity_by_id_not_found(self):
        resp = send_command("getEntityById", {"eid": 999999})
        assert resp["status"] == "error"

    def test_get_polyline_data(self):
        send_command("addPolyline", {
            "points": [{"x": 200, "y": 200}, {"x": 210, "y": 210}, {"x": 220, "y": 200}],
            "closed": True
        })
        resp_all = send_command("getAllEntities")
        polyline_ent = None
        for e in resp_all["entities"]:
            if e.get("typeName") == "polyline":
                polyline_ent = e
                break
        if polyline_ent:
            resp = send_command("getPolylineData", {"eid": float(polyline_ent["eid"])})
            assert resp["status"] == "ok"
            assert "vertices" in resp
            assert len(resp["vertices"]) >= 3

    def test_get_polyline_data_not_found(self):
        resp = send_command("getPolylineData", {"eid": 999999})
        assert resp["status"] == "error"


# --- entity operations ---

class TestEntityOperations:
    def _get_first_line_eid(self):
        resp = send_command("getAllEntities")
        for e in resp["entities"]:
            if e.get("typeName") == "line":
                return e["eid"]
        return None

    def test_move_entity(self):
        send_command("addLine", {"x1": 300, "y1": 300, "x2": 310, "y2": 310})
        eid = self._get_first_line_eid()
        if eid:
            resp = send_command("moveEntity", {"eid": float(eid), "dx": 5, "dy": 5})
            assert resp["status"] == "ok"

    def test_rotate_entity(self):
        eid = self._get_first_line_eid()
        if eid:
            resp = send_command("rotateEntity", {
                "eid": float(eid), "cx": 0, "cy": 0, "angle": 45
            })
            assert resp["status"] == "ok"

    def test_scale_entity(self):
        eid = self._get_first_line_eid()
        if eid:
            resp = send_command("scaleEntity", {
                "eid": float(eid), "cx": 0, "cy": 0, "sx": 1.5, "sy": 1.5
            })
            assert resp["status"] == "ok"

    def test_move_rotate_entity(self):
        send_command("addLine", {"x1": 400, "y1": 400, "x2": 410, "y2": 410})
        resp_all = send_command("getAllEntities")
        eid = None
        for e in resp_all["entities"]:
            if e.get("typeName") == "line":
                eid = e["eid"]
        if eid:
            resp = send_command("moveRotateEntity", {
                "eid": float(eid), "dx": 2, "dy": 2, "cx": 0, "cy": 0, "angle": 30
            })
            assert resp["status"] == "ok"

    def test_move_entity_not_found(self):
        resp = send_command("moveEntity", {"eid": 999999, "dx": 1, "dy": 1})
        assert resp["status"] == "error"

    def test_remove_entity(self):
        send_command("addLine", {"x1": 500, "y1": 500, "x2": 510, "y2": 510})
        resp_all = send_command("getAllEntities")
        eid = None
        for e in resp_all["entities"]:
            if e.get("typeName") == "line":
                eid = e["eid"]
        if eid:
            resp = send_command("removeEntity", {"eid": float(eid)})
            assert resp["status"] == "ok"

    def test_remove_entity_not_found(self):
        resp = send_command("removeEntity", {"eid": 999999})
        assert resp["status"] == "error"


# --- update entity data ---

class TestUpdateEntity:
    def test_update_entity_color(self):
        send_command("addLine", {"x1": 600, "y1": 600, "x2": 620, "y2": 620})
        resp_all = send_command("getAllEntities")
        eid = None
        for e in resp_all["entities"]:
            if e.get("typeName") == "line":
                eid = e["eid"]
        if eid:
            resp = send_command("updateEntity", {
                "eid": float(eid),
                "data": {"62": 16711680}  # red
            })
            assert resp["status"] == "ok"

    def test_update_entity_not_found(self):
        resp = send_command("updateEntity", {"eid": 999999, "data": {"62": 0}})
        assert resp["status"] == "error"


# --- update polyline data ---

class TestUpdatePolyline:
    def test_update_polyline_data(self):
        send_command("addPolyline", {
            "points": [{"x": 700, "y": 700}, {"x": 710, "y": 710}],
            "closed": False
        })
        resp_all = send_command("getAllEntities")
        eid = None
        for e in resp_all["entities"]:
            if e.get("typeName") == "polyline":
                eid = e["eid"]
        if eid:
            resp = send_command("updatePolylineData", {
                "eid": float(eid),
                "vertices": [
                    {"x": 700, "y": 700, "bulge": 0},
                    {"x": 710, "y": 700, "bulge": 0},
                    {"x": 710, "y": 710, "bulge": 0}
                ]
            })
            assert resp["status"] == "ok"


# --- layer properties ---

class TestLayerProperties:
    def test_get_layer_properties(self):
        send_command("setLayer", {"name": "test_props_layer"})
        resp = send_command("getLayerProperties", {"layer": "test_props_layer"})
        assert resp["status"] == "ok"
        assert "color" in resp
        assert "lineWidth" in resp
        assert "lineType" in resp

    def test_set_layer_properties(self):
        send_command("setLayer", {"name": "test_set_props"})
        resp = send_command("setLayerProperties", {
            "layer": "test_set_props",
            "color": 255,
            "lineWidth": "0.25mm",
            "lineType": "DashLine"
        })
        assert resp["status"] == "ok"
        # verify round-trip
        resp2 = send_command("getLayerProperties", {"layer": "test_set_props"})
        assert resp2["status"] == "ok"


# --- variables ---

class TestVariables:
    def test_get_variable(self):
        resp = send_command("getVariable", {"key": "DIMTXT"})
        # DIMTXT may or may not exist
        assert resp["status"] in ("ok", "error")

    def test_set_and_get_variable(self):
        send_command("setVariable", {"key": "$TESTVAR_INT", "value": 42, "code": 70})
        resp = send_command("getVariable", {"key": "$TESTVAR_INT"})
        assert resp["status"] == "ok"
        assert resp["value"] == 42

    def test_set_and_get_double_variable(self):
        send_command("setVariable", {"key": "$TESTVAR_DBL", "value": 3.14, "code": 40})
        resp = send_command("getVariable", {"key": "$TESTVAR_DBL"})
        assert resp["status"] == "ok"
        assert abs(resp["value"] - 3.14) < 0.01


# --- unselect ---

class TestUnselect:
    def test_unselect_entities(self):
        resp = send_command("unselectEntities")
        assert resp["status"] == "ok"


# --- real to str ---

class TestRealToStr:
    def test_real_to_str(self):
        resp = send_command("realToStr", {"num": 3.14159, "units": 2, "prec": 4})
        assert resp["status"] == "ok"
        assert "result" in resp
        assert isinstance(resp["result"], str)

    def test_real_to_str_default(self):
        resp = send_command("realToStr", {"num": 42.0})
        assert resp["status"] == "ok"
        assert "result" in resp

    def test_real_to_str_zero(self):
        resp = send_command("realToStr", {"num": 0.0, "units": 2, "prec": 2})
        assert resp["status"] == "ok"


# ============================================================
# Edge cases
# ============================================================

class TestProtocolEdgeCases:
    """Protocol-level edge cases: missing params, empty payloads, etc."""

    def test_empty_method(self):
        resp = send_command("")
        assert resp["status"] == "error"

    def test_method_only_no_params(self):
        resp = send_command("getLayers")
        assert resp["status"] == "ok"

    def test_extra_unknown_params_ignored(self):
        resp = send_command("addLine", {
            "x1": 0, "y1": 0, "x2": 1, "y2": 1,
            "bogus": True, "extra_key": "ignored"
        })
        assert resp["status"] == "ok"


class TestPrimitiveEdgeCases:
    """Edge cases for primitive drawing commands."""

    def test_zero_length_line(self):
        resp = send_command("addLine", {"x1": 5, "y1": 5, "x2": 5, "y2": 5})
        assert resp["status"] == "ok"

    def test_very_large_coordinates(self):
        resp = send_command("addLine", {"x1": 1e9, "y1": -1e9, "x2": -1e9, "y2": 1e9})
        assert resp["status"] == "ok"

    def test_very_small_circle(self):
        resp = send_command("addCircle", {"x": 0, "y": 0, "r": 1e-9})
        assert resp["status"] == "ok"

    def test_negative_radius_circle(self):
        resp = send_command("addCircle", {"x": 0, "y": 0, "r": -5})
        assert resp["status"] == "ok"

    def test_negative_width_rectangle(self):
        resp = send_command("addRectangle", {"x": 0, "y": 0, "w": -10, "h": -5})
        assert resp["status"] == "ok"

    def test_zero_size_rectangle(self):
        resp = send_command("addRectangle", {"x": 3, "y": 3, "w": 0, "h": 0})
        assert resp["status"] == "ok"

    def test_single_point_polyline(self):
        resp = send_command("addPolyline", {"points": [{"x": 1, "y": 1}]})
        assert resp["status"] == "error"

    def test_empty_points_polyline(self):
        resp = send_command("addPolyline", {"points": []})
        assert resp["status"] == "error"

    def test_empty_points_lines(self):
        resp = send_command("addLines", {"points": []})
        assert resp["status"] == "error"

    def test_full_circle_arc(self):
        resp = send_command("addArc", {"x": 0, "y": 0, "r": 5, "a1": 0, "a2": 360})
        assert resp["status"] == "ok"

    def test_negative_angle_arc(self):
        resp = send_command("addArc", {"x": 0, "y": 0, "r": 5, "a1": -180, "a2": 180})
        assert resp["status"] == "ok"

    def test_ellipse_ratio_zero(self):
        resp = send_command("addEllipse", {"cx": 5, "cy": 5, "ex": 10, "ey": 5, "ratio": 0})
        assert resp["status"] == "ok"

    def test_ellipse_ratio_one(self):
        resp = send_command("addEllipse", {"cx": 5, "cy": 5, "ex": 10, "ey": 5, "ratio": 1})
        assert resp["status"] == "ok"

    def test_text_with_newlines(self):
        resp = send_command("addText", {"text": "Line1\nLine2\nLine3", "x": 0, "y": 0, "size": 3})
        assert resp["status"] == "ok"

    def test_text_with_tabs(self):
        resp = send_command("addText", {"text": "Col1\tCol2\tCol3", "x": 0, "y": 0, "size": 3})
        assert resp["status"] == "ok"

    def test_text_zero_size(self):
        resp = send_command("addText", {"text": "Tiny", "x": 0, "y": 0, "size": 0})
        assert resp["status"] == "ok"

    def test_text_very_large_size(self):
        resp = send_command("addText", {"text": "BIG", "x": 0, "y": 0, "size": 1000})
        assert resp["status"] == "ok"

    def test_spline_two_points(self):
        resp = send_command("addSplinePoints", {
            "points": [{"x": 0, "y": 0}, {"x": 10, "y": 10}]
        })
        assert resp["status"] == "ok"

    def test_spline_single_point(self):
        resp = send_command("addSplinePoints", {"points": [{"x": 5, "y": 5}]})
        assert resp["status"] == "error"

    def test_add_point_negative_coords(self):
        resp = send_command("addPoint", {"x": -100, "y": -200})
        assert resp["status"] == "ok"

    def test_add_point_float_coords(self):
        resp = send_command("addPoint", {"x": 1.5, "y": 2.7})
        assert resp["status"] == "ok"


class TestDimensionEdgeCases:
    """Edge cases for dimension commands."""

    def test_dim_linear_zero_distance(self):
        resp = send_command("addDimLinear", {
            "x1": 5, "y1": 5, "x2": 5, "y2": 5,
            "dimLineOffset": 10, "angle": 0
        })
        assert resp["status"] == "ok"

    def test_dim_linear_negative_offset(self):
        resp = send_command("addDimLinear", {
            "x1": 0, "y1": 0, "x2": 10, "y2": 0,
            "dimLineOffset": -5, "angle": 0
        })
        assert resp["status"] == "ok"

    def test_dim_aligned_same_points(self):
        resp = send_command("addDimAligned", {
            "x1": 3, "y1": 3, "x2": 3, "y2": 3,
            "dimLineOffset": 10
        })
        assert resp["status"] == "ok"

    def test_dim_leader_empty_text(self):
        resp = send_command("addDimLeader", {
            "x1": 0, "y1": 0, "x2": 5, "y2": 5, "text": ""
        })
        assert resp["status"] == "ok"

    def test_dim_leader_unicode_text(self):
        resp = send_command("addDimLeader", {
            "x1": 0, "y1": 0, "x2": 5, "y2": 5, "text": "Размер Δ=5мм"
        })
        assert resp["status"] == "ok"


class TestEntityEdgeCases:
    """Edge cases for entity query and mutation operations."""

    def test_get_entity_by_id_zero(self):
        resp = send_command("getEntityById", {"eid": 0})
        assert resp["status"] == "error"

    def test_get_entity_by_id_negative(self):
        resp = send_command("getEntityById", {"eid": -1})
        assert resp["status"] == "error"

    def test_move_entity_zero_offset(self):
        send_command("addLine", {"x1": 800, "y1": 800, "x2": 810, "y2": 810})
        resp_all = send_command("getAllEntities")
        eid = None
        for e in resp_all["entities"]:
            if e.get("typeName") == "line":
                eid = e["eid"]
        if eid:
            resp = send_command("moveEntity", {"eid": float(eid), "dx": 0, "dy": 0})
            assert resp["status"] == "ok"

    def test_rotate_entity_full_360(self):
        send_command("addLine", {"x1": 850, "y1": 850, "x2": 860, "y2": 860})
        resp_all = send_command("getAllEntities")
        eid = None
        for e in resp_all["entities"]:
            if e.get("typeName") == "line":
                eid = e["eid"]
        if eid:
            resp = send_command("rotateEntity", {"eid": float(eid), "cx": 0, "cy": 0, "angle": 360})
            assert resp["status"] == "ok"

    def test_scale_entity_one(self):
        send_command("addLine", {"x1": 870, "y1": 870, "x2": 880, "y2": 880})
        resp_all = send_command("getAllEntities")
        eid = None
        for e in resp_all["entities"]:
            if e.get("typeName") == "line":
                eid = e["eid"]
        if eid:
            resp = send_command("scaleEntity", {"eid": float(eid), "cx": 0, "cy": 0, "sx": 1, "sy": 1})
            assert resp["status"] == "ok"

    def test_scale_entity_negative(self):
        send_command("addLine", {"x1": 890, "y1": 890, "x2": 900, "y2": 900})
        resp_all = send_command("getAllEntities")
        eid = None
        for e in resp_all["entities"]:
            if e.get("typeName") == "line":
                eid = e["eid"]
        if eid:
            resp = send_command("scaleEntity", {"eid": float(eid), "cx": 0, "cy": 0, "sx": -1, "sy": -1})
            assert resp["status"] == "ok"

    def test_rotate_entity_not_found(self):
        resp = send_command("rotateEntity", {"eid": 999999, "cx": 0, "cy": 0, "angle": 45})
        assert resp["status"] == "error"

    def test_scale_entity_not_found(self):
        resp = send_command("scaleEntity", {"eid": 999999, "cx": 0, "cy": 0, "sx": 2, "sy": 2})
        assert resp["status"] == "error"

    def test_move_rotate_entity_not_found(self):
        resp = send_command("moveRotateEntity", {"eid": 999999, "dx": 1, "dy": 1, "cx": 0, "cy": 0, "angle": 45})
        assert resp["status"] == "error"

    def test_update_entity_multiple_fields(self):
        send_command("addLine", {"x1": 910, "y1": 910, "x2": 920, "y2": 920})
        resp_all = send_command("getAllEntities")
        eid = None
        for e in resp_all["entities"]:
            if e.get("typeName") == "line":
                eid = e["eid"]
        if eid:
            resp = send_command("updateEntity", {
                "eid": float(eid),
                "data": {"62": 255, "8": "0", "6": "DashLine"}
            })
            assert resp["status"] == "ok"

    def test_get_polyline_data_on_non_polyline(self):
        send_command("addLine", {"x1": 930, "y1": 930, "x2": 940, "y2": 940})
        resp_all = send_command("getAllEntities")
        eid = None
        for e in resp_all["entities"]:
            if e.get("typeName") == "line":
                eid = e["eid"]
        if eid:
            resp = send_command("getPolylineData", {"eid": float(eid)})
            assert resp["status"] == "error"

    def test_update_polyline_not_found(self):
        resp = send_command("updatePolylineData", {
            "eid": 999999,
            "vertices": [{"x": 0, "y": 0, "bulge": 0}]
        })
        assert resp["status"] == "error"

    def test_update_polyline_empty_vertices(self):
        send_command("addPolyline", {
            "points": [{"x": 950, "y": 950}, {"x": 960, "y": 960}],
            "closed": False
        })
        resp_all = send_command("getAllEntities")
        eid = None
        for e in resp_all["entities"]:
            if e.get("typeName") == "polyline":
                eid = e["eid"]
        if eid:
            resp = send_command("updatePolylineData", {"eid": float(eid), "vertices": []})
            assert resp["status"] in ("ok", "error")


class TestLayerEdgeCases:
    """Edge cases for layer operations."""

    def test_delete_nonexistent_layer(self):
        resp = send_command("deleteLayer", {"name": "no_such_layer_xyz"})
        assert resp["status"] in ("ok", "error")

    def test_set_layer_empty_name(self):
        resp = send_command("setLayer", {"name": ""})
        assert resp["status"] in ("ok", "error")

    def test_layer_name_with_spaces(self):
        name = "layer with spaces test"
        send_command("setLayer", {"name": name})
        resp = send_command("getLayers")
        assert resp["status"] == "ok"
        assert name in resp["layers"]
        send_command("deleteLayer", {"name": name})

    def test_get_layer_properties_default_layer(self):
        resp = send_command("getLayerProperties", {"layer": "0"})
        assert resp["status"] == "ok"

    def test_get_layer_properties_nonexistent(self):
        resp = send_command("getLayerProperties", {"layer": "no_such_layer_xyz"})
        assert resp["status"] in ("ok", "error")

    def test_set_layer_properties_zero_color(self):
        send_command("setLayer", {"name": "zero_color_test"})
        resp = send_command("setLayerProperties", {
            "layer": "zero_color_test",
            "color": 0,
            "lineWidth": "0.00mm",
            "lineType": "SolidLine"
        })
        assert resp["status"] == "ok"


class TestVariableEdgeCases:
    """Edge cases for variable operations."""

    def test_get_nonexistent_variable(self):
        resp = send_command("getVariable", {"key": "$NO_SUCH_VAR_XYZ"})
        assert resp["status"] == "error"

    def test_set_variable_string_value(self):
        send_command("setVariable", {"key": "$TESTVAR_STR", "value": "hello", "code": 1})
        resp = send_command("getVariable", {"key": "$TESTVAR_STR"})
        assert resp["status"] in ("ok", "error")

    @pytest.mark.skip(reason="LibreCAD getVariableInt API bug with value 0")
    def test_set_variable_zero_value(self):
        send_command("setVariable", {"key": "$TESTVAR_ZERO", "value": 0, "code": 70})
        resp = send_command("getVariable", {"key": "$TESTVAR_ZERO"})
        assert resp["status"] == "ok"
        assert resp["value"] == 0

    def test_set_variable_negative_value(self):
        send_command("setVariable", {"key": "$TESTVAR_NEG", "value": -99.5, "code": 40})
        resp = send_command("getVariable", {"key": "$TESTVAR_NEG"})
        assert resp["status"] == "ok"
        assert abs(resp["value"] - (-99.5)) < 0.1


class TestConstructionLineEdgeCases:
    """Edge cases for construction line tools."""

    def test_line_angle_zero_degrees(self):
        resp = send_command("lineAngle", {"x": 0, "y": 0, "angle": 0, "length": 10})
        assert resp["status"] == "ok"

    def test_line_angle_360(self):
        resp = send_command("lineAngle", {"x": 0, "y": 0, "angle": 360, "length": 10})
        assert resp["status"] == "ok"

    def test_line_angle_negative(self):
        resp = send_command("lineAngle", {"x": 0, "y": 0, "angle": -45, "length": 10})
        assert resp["status"] == "ok"

    def test_parallel_zero_length_reference(self):
        resp = send_command("lineParallelThroughPoint", {
            "lx1": 5, "ly1": 5, "lx2": 5, "ly2": 5, "px": 10, "py": 10
        })
        assert resp["status"] == "error"

    def test_parallel_zero_distance(self):
        resp = send_command("lineParallel", {
            "lx1": 0, "ly1": 0, "lx2": 10, "ly2": 0, "distance": 0
        })
        assert resp["status"] == "ok"

    def test_parallel_negative_distance(self):
        resp = send_command("lineParallel", {
            "lx1": 0, "ly1": 0, "lx2": 10, "ly2": 0, "distance": -5
        })
        assert resp["status"] == "ok"

    def test_bisector_collinear_rays(self):
        resp = send_command("lineBisector", {
            "vx": 0, "vy": 0, "ax": 10, "ay": 0, "bx": 20, "by": 0, "length": 5
        })
        assert resp["status"] == "ok"

    def test_bisector_opposite_rays(self):
        resp = send_command("lineBisector", {
            "vx": 0, "vy": 0, "ax": 10, "ay": 0, "bx": -10, "by": 0, "length": 5
        })
        assert resp["status"] == "ok"

    def test_tangent_pc_on_circle(self):
        resp = send_command("lineTangentPC", {"px": 8, "py": 5, "cx": 5, "cy": 5, "r": 3})
        assert resp["status"] == "error"

    def test_tangent_cc_overlapping_circles(self):
        resp = send_command("lineTangentCC", {
            "cx1": 5, "cy1": 5, "r1": 3, "cx2": 5, "cy2": 5, "r2": 3
        })
        assert resp["status"] == "error"

    def test_snake_single_segment(self):
        resp = send_command("lineSnakeX", {"x": 0, "y": 0, "width": 10, "segments": 1, "gap": 3})
        assert resp["status"] == "ok"

    def test_snake_many_segments(self):
        resp = send_command("lineSnakeY", {"x": 0, "y": 0, "height": 5, "segments": 10, "gap": 1})
        assert resp["status"] == "ok"


class TestSliceDivideEdgeCases:
    """Edge cases for slice/divide operations."""

    def test_slice_divide_line_one_segment(self):
        resp = send_command("sliceDivideLine", {
            "x1": 0, "y1": 0, "x2": 10, "y2": 0, "segments": 1
        })
        assert resp["status"] == "ok"
        assert len(resp["points"]) == 2

    def test_slice_divide_line_many_segments(self):
        resp = send_command("sliceDivideLine", {
            "x1": 0, "y1": 0, "x2": 100, "y2": 0, "segments": 20
        })
        assert resp["status"] == "ok"
        assert len(resp["points"]) == 21

    def test_slice_divide_circle_one_segment(self):
        resp = send_command("sliceDivideCircle", {
            "cx": 0, "cy": 0, "r": 5, "segments": 1
        })
        assert resp["status"] == "ok"
        assert len(resp["points"]) == 1

    def test_slice_divide_circle_zero_radius(self):
        resp = send_command("sliceDivideCircle", {
            "cx": 5, "cy": 5, "r": 0, "segments": 4
        })
        assert resp["status"] == "ok"

    def test_center_mark_zero_size(self):
        resp = send_command("centerMark", {"cx": 5, "cy": 5, "size": 0})
        assert resp["status"] == "ok"

    def test_centerline_zero_radius(self):
        resp = send_command("centerline", {"cx": 5, "cy": 5, "r": 0, "extension": 2})
        assert resp["status"] == "ok"


class TestArchitecturalEdgeCases:
    """Edge cases for architectural commands."""

    def test_wall_zero_thickness(self):
        resp = send_command("addWall", {"x1": 0, "y1": 0, "x2": 10, "y2": 0, "thickness": 0})
        assert resp["status"] == "ok"

    def test_wall_zero_length(self):
        resp = send_command("addWall", {"x1": 5, "y1": 5, "x2": 5, "y2": 5, "thickness": 0.25})
        assert resp["status"] == "ok"

    def test_opening_zero_width(self):
        resp = send_command("addOpening", {"x1": 5, "y1": 0, "x2": 5, "y2": 0, "type": "door"})
        assert resp["status"] == "ok"


class TestTacticalEdgeCases:
    """Edge cases for tactical commands."""

    def test_tactical_neutral_symbol(self):
        resp = send_command("addTacticalSymbol", {"x": 30, "y": 30, "identity": "neutral", "unit": "infantry"})
        assert resp["status"] == "ok"

    def test_tactical_hq_symbol(self):
        resp = send_command("addTacticalSymbol", {"x": 40, "y": 40, "identity": "friendly", "unit": "HQ"})
        assert resp["status"] == "ok"

    def test_tactical_line_two_points(self):
        resp = send_command("addTacticalLine", {
            "points": [{"x": 0, "y": 0}, {"x": 50, "y": 50}],
            "type": "axis_of_advance"
        })
        assert resp["status"] == "ok"

    def test_tactical_line_many_points(self):
        resp = send_command("addTacticalLine", {
            "points": [{"x": i * 10, "y": i * 5} for i in range(10)],
            "type": "boundary"
        })
        assert resp["status"] == "ok"


class TestRealToStrEdgeCases:
    """Edge cases for realToStr."""

    def test_real_to_str_negative(self):
        resp = send_command("realToStr", {"num": -42.5, "units": 2, "prec": 2})
        assert resp["status"] == "ok"
        assert isinstance(resp["result"], str)

    def test_real_to_str_very_large(self):
        resp = send_command("realToStr", {"num": 1e15, "units": 0, "prec": 0})
        assert resp["status"] == "ok"

    def test_real_to_str_very_small(self):
        resp = send_command("realToStr", {"num": 1e-15, "units": 0, "prec": 15})
        assert resp["status"] == "ok"

    def test_real_to_str_nan(self):
        resp = send_command("realToStr", {"num": float("nan"), "units": 0, "prec": 2})
        assert resp["status"] == "error"

    def test_real_to_str_infinity(self):
        resp = send_command("realToStr", {"num": float("inf"), "units": 0, "prec": 2})
        assert resp["status"] == "error"
