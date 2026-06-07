import asyncio
import json
import socket
from mcp.server.models import InitializationOptions
from mcp.server import NotificationOptions, Server
from mcp.server.stdio import stdio_server
import mcp.types as types

# LibreCAD TCP configuration
LIBRECAD_HOST = "localhost"
LIBRECAD_PORT = 12346

server = Server("librecad-mcp")

# Full list of supported methods for discovery
SUPPORTED_METHODS = {
    "Primitives": ["addLine", "addCircle", "addArc", "addEllipse", "addPolyline", "addLines", "addPoint", "addSplinePoints", "addRectangle", "addSolid", "addConstructionLine", "addHatch", "addText", "addMText", "addInsert"],
    "Architectural": ["addWall", "addOpening", "addWindow", "calculateArea", "trimLine", "joinWalls"],
    "Interactive": ["getPoint", "getEntity", "getSelection", "getString", "getInt", "getReal"],
    "Geometry Tools": ["line2Points", "lineAngle", "lineHorizontal", "lineVertical", "lineParallel", "lineBisector", "lineTangentPC", "lineTangentCC", "lineOrthogonal", "lineRelativeAngle", "lineSnakeX", "lineSnakeY"],
    "Entity Operations": ["moveEntity", "rotateEntity", "scaleEntity", "moveRotateEntity", "offsetEntity", "removeEntity", "updateEntity", "getAllEntities", "getEntityById"],
    "Layers/Props": ["setLayer", "deleteLayer", "getLayers", "getBlocks", "getLayerProperties", "setLayerProperties", "getVariable", "setVariable"]
}

def send_librecad_command(method, params):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(10.0) # Increased for interactive commands
            s.connect((LIBRECAD_HOST, LIBRECAD_PORT))
            payload = json.dumps({"method": method, "params": params})
            s.sendall(payload.encode('utf-8'))
            data = s.recv(65536) # Larger buffer for getAllEntities
            if not data:
                return {"status": "error", "message": "No data received"}
            return json.loads(data.decode('utf-8'))
    except Exception as e:
        return {"status": "error", "message": str(e)}

@server.list_tools()
async def handle_list_tools() -> list[types.Tool]:
    return [
        types.Tool(
            name="librecad_invoke",
            description="Invoke any LibreCAD method. See librecad_list_methods for valid method names.",
            inputSchema={
                "type": "object",
                "properties": {
                    "method": {"type": "string"},
                    "params": {"type": "object"}
                },
                "required": ["method", "params"]
            }
        ),
        types.Tool(
            name="librecad_list_methods",
            description="Returns a full list of available LibreCAD drawing and manipulation methods.",
            inputSchema={"type": "object", "properties": {}}
        ),
        types.Tool(
            name="librecad_get_entities",
            description="Get all entities from the current drawing.",
            inputSchema={"type": "object", "properties": {}}
        )
    ]

@server.call_tool()
async def handle_call_tool(name: str, arguments: dict | None) -> list[types.TextContent]:
    if name == "librecad_invoke":
        result = send_librecad_command(arguments.get("method"), arguments.get("params", {}))
        return [types.TextContent(type="text", text=json.dumps(result, indent=2))]
    elif name == "librecad_list_methods":
        return [types.TextContent(type="text", text=json.dumps(SUPPORTED_METHODS, indent=2))]
    elif name == "librecad_get_entities":
        result = send_librecad_command("getAllEntities", {})
        return [types.TextContent(type="text", text=json.dumps(result, indent=2))]
    raise ValueError(f"Unknown tool: {name}")

async def main():
    async with stdio_server() as (r, w):
        await server.run(
            r, 
            w, 
            InitializationOptions(
                server_name="librecad-mcp", 
                server_version="1.1.0", 
                capabilities=server.get_capabilities(
                    notification_options=NotificationOptions(),
                    experimental_capabilities={},
                )
            )
        )

if __name__ == "__main__":
    asyncio.run(main())
