"""Exercise the real stdio MCP server; run with its installed requirements."""
from pathlib import Path
import sys
import asyncio
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

async def main():
    server = StdioServerParameters(command=sys.executable, args=[str(Path(__file__).resolve().parents[1] / 'standalone-tools/toern-mcp/server.py')])
    async with stdio_client(server) as (read, write):
        async with ClientSession(read, write) as client:
            await client.initialize()
            tools = await client.list_tools()
            names = {t.name for t in tools.tools}
            assert names == {'list_devices','connect','disconnect','get_state','get_display','turn_encoder','set_control','press_control','perform_gesture','release_controls','get_menu','get_filters','reset_filters','get_matrix'}, names
            result = await client.call_tool('get_state', {})
            assert result.isError
            result = await client.call_tool('disconnect', {})
            assert not result.isError
            print('PASS: MCP initialize, tool schemas, disconnected error, disconnect over real stdio')

asyncio.run(main())
