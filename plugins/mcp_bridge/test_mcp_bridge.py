import socket
import json
import time

def send_command(cmd):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(("localhost", 12346))
            s.sendall(json.dumps(cmd).encode('utf-8'))
            data = s.recv(4096)
            return json.loads(data.decode('utf-8'))
    except Exception as e:
        return {"status": "error", "message": str(e)}

def test_bridge():
    print("--- Testing addMText ---")
    resp = send_command({
        "method": "addMText",
        "params": {
            "text": "Hello MCP Bridge\nMulti-line",
            "x": 10, "y": 10,
            "height": 5.0, "angle": 0.0
        }
    })
    print("Response:", resp)

    print("\n--- Testing addLine (for offset) ---")
    resp = send_command({
        "method": "addLine",
        "params": {"x1": 50, "y1": 50, "x2": 150, "y2": 50}
    })
    print("Response:", resp)
    
    # We need EID to test offset. Let's get all entities.
    print("\n--- Getting EID for offset ---")
    all_ents = send_command({"method": "getAllEntities", "params": {}})
    eid = None
    if all_ents.get("status") == "ok":
        # Look for the last line
        for ent in reversed(all_ents.get("entities", [])):
            if ent.get("typeName") == "line":
                eid = int(ent.get("eid"))
                break
    
    if eid:
        print(f"Testing offsetEntity for EID: {eid}")
        resp = send_command({
            "method": "offsetEntity",
            "params": {"eid": eid, "distance": 10.0}
        })
        print("Response:", resp)
    else:
        print("Could not find entity for offset test")

    print("\n--- Testing addHatch ---")
    resp = send_command({
        "method": "addHatch",
        "params": {
            "points": [
                {"x": 200, "y": 200},
                {"x": 300, "y": 200},
                {"x": 300, "y": 300},
                {"x": 200, "y": 300}
            ],
            "angle": 45.0,
            "distance": 5.0
        }
    })
    print("Response:", resp)

if __name__ == "__main__":
    test_bridge()
