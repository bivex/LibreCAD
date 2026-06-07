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

def test_cyrillic():
    print("--- Testing Cyrillic Text (RU/UK) ---")
    
    # Test strings
    ru_text = "Привет, LibreCAD! (Russian)"
    uk_text = "Привіт, LibreCAD! (Ukrainian) з символами і, є, є, ї, ґ"
    
    # 1. Simple Text (Default font is now unicode for Cyrillic-safe rendering)
    print("\n1. Drawing Simple Text (Russian)...")
    resp = send_command({
        "method": "addText",
        "params": {
            "text": ru_text,
            "x": 0, "y": -100,
            "size": 20,
            "font": "unicode"
        }
    })
    print("Response:", resp)

    # 2. Multi-line MText (Ukrainian)
    print("\n2. Drawing Multi-line MText (Ukrainian)...")
    resp = send_command({
        "method": "addMText",
        "params": {
            "text": uk_text + "\nДругий рядок тексту.",
            "x": 0, "y": -200,
            "height": 15,
            "font": "unicode"
        }
    })
    print("Response:", resp)

    # 3. calculateArea with Cyrillic label
    print("\n3. Testing calculateArea with Cyrillic Label...")
    resp = send_command({
        "method": "calculateArea",
        "params": {
            "points": [
                {"x": 1000, "y": -500},
                {"x": 3000, "y": -500},
                {"x": 3000, "y": -1500},
                {"x": 1000, "y": -1500}
            ],
            "unit": "mm",
            "label": "Вітальня (Area Test)",
            "textSize": 100,
            "font": "unicode"
        }
    })
    print("Response:", resp)

if __name__ == "__main__":
    test_cyrillic()
