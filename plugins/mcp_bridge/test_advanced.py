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

def test_advanced():
    print("--- Testing addConstructionLine ---")
    resp = send_command({
        "method": "addConstructionLine",
        "params": {"x1": 0, "y1": 0, "x2": 100, "y2": 100}
    })
    print("Response:", resp)

    print("\n--- Testing Native Copying (moveEntity with copy=true) ---")
    # Create a circle to copy
    send_command({"method": "addCircle", "params": {"x": 200, "y": 50, "r": 20}})
    all_ents = send_command({"method": "getAllEntities", "params": {}})
    eid = None
    for ent in reversed(all_ents.get("entities", [])):
        if ent.get("typeName") == "circle":
            eid = int(ent.get("eid"))
            break
    
    if eid:
        print(f"Copying circle EID {eid} to (250, 50)")
        resp = send_command({
            "method": "moveEntity",
            "params": {"eid": eid, "dx": 50, "dy": 0, "copy": True}
        })
        print("Response:", resp)
    
    print("\n--- Testing addSolid (Triangle) ---")
    resp = send_command({
        "method": "addSolid",
        "params": {
            "points": [
                {"x": 300, "y": 50},
                {"x": 350, "y": 50},
                {"x": 325, "y": 100}
            ]
        }
    })
    print("Response:", resp)

    print("\n--- Testing Interactive getPoint (LOOK AT LIBRECAD AND CLICK) ---")
    print("Waiting for your click in LibreCAD...")
    resp = send_command({
        "method": "getPoint",
        "params": {"message": "Click somewhere to test MCP!"}
    })
    print("You clicked at:", resp)

if __name__ == "__main__":
    test_advanced()
