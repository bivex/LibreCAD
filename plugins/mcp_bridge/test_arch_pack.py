import socket
import json
import time

def send_command(cmd):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(5)
            s.connect(("localhost", 12346))
            s.sendall(json.dumps(cmd).encode('utf-8'))
            data = s.recv(4096)
            return json.loads(data.decode('utf-8'))
    except Exception as e:
        return {"status": "error", "message": str(e)}

def test_architectural():
    print("--- 1. Testing calculateArea ---")
    # Draw a 5m x 4m room (in mm)
    room_pts = [
        {"x": 0, "y": 0},
        {"x": 5000, "y": 0},
        {"x": 5000, "y": 4000},
        {"x": 0, "y": 4000}
    ]
    resp = send_command({
        "method": "calculateArea",
        "params": {
            "points": room_pts,
            "unit": "mm",
            "label": "Living Room",
            "textSize": 250
        }
    })
    print("Area Response:", resp)
    if resp.get("status") == "ok":
        print(f"Calculated Area: {resp.get('area_m2')} m2")

    print("\n--- 2. Testing addWindow ---")
    # Add a window in the middle of the 5m wall
    resp = send_command({
        "method": "addWindow",
        "params": {
            "x": 2500, "y": 0,
            "width": 1500,
            "depth": 300,
            "sill": 100,
            "angle": 0,
            "layer": "Windows"
        }
    })
    print("Window Response:", resp)

    print("\n--- 3. Testing joinWalls (Corner) ---")
    # Create two intersecting walls (as pairs of lines)
    # Wall 1 (Vertical)
    w1_inner = send_command({"method": "addLine", "params": {"x1": 1000, "y1": 0, "x2": 1000, "y2": 2000}})
    w1_outer = send_command({"method": "addLine", "params": {"x1": 1200, "y1": 0, "x2": 1200, "y2": 2200}})
    
    # Wall 2 (Horizontal)
    w2_inner = send_command({"method": "addLine", "params": {"x1": 0, "y1": 1000, "x2": 2000, "y2": 1000}})
    w2_outer = send_command({"method": "addLine", "params": {"x1": 0, "y1": 1200, "x2": 2200, "y2": 1200}})

    # Get EIDs from LibreCAD (needs getAllEntities because addLine doesn't return EID directly yet)
    all_ents = send_command({"method": "getAllEntities", "params": {}})
    lines = [e for e in all_ents.get("entities", []) if e.get("typeName") == "line"]
    
    if len(lines) >= 4:
        # Take last 4 lines created
        eids = [int(l["eid"]) for l in lines[-4:]]
        print(f"Joining walls with EIDs: {eids}")
        resp = send_command({
            "method": "joinWalls",
            "params": {
                "wall1_eids": [eids[0], eids[1]],
                "wall2_eids": [eids[2], eids[3]]
            }
        })
        print("Join Walls Response:", resp)
    else:
        print("Not enough lines found to test joinWalls")

if __name__ == "__main__":
    test_architectural()
